#pragma once

#include <mutex>
#include <thread>
#include <atomic>

#include "util/clamp.h"

namespace Heat
{
    struct OutsideTemp
    {
        int actual;
        int effective;
        int actualToExpInnerDelta;

        OutsideTemp()
            : actual(0), effective(0), actualToExpInnerDelta(0)
        {
        }

        OutsideTemp(int anActual, int anActualToExpInnerDelta)
            : actual(anActual), actualToExpInnerDelta(anActualToExpInnerDelta)
        {
            // TODO: to config?
            const int innerDeltaCorrection = 5;
            effective = actual + innerDeltaCorrection * actualToExpInnerDelta;
        }

        operator int() const
        {
            return effective;
        }

        std::string toString() const
        {
            return std::string("{ effective=") + std::to_string(effective)
                + " actual=" + std::to_string(actual)
                + " inner delta=" + std::to_string(actualToExpInnerDelta)
                + " }";
        }
    };

	class MixTempApprox
	{
		std::mutex _mutex;
		std::vector< std::pair< int, int > > _mixTemps;

		void _updateFromCfg(const Config &cfg)
		{
			std::vector< std::pair< int, int > > newTemps;

			for (const auto &i : cfg.mixTemp)
				newTemps.push_back(std::make_pair(i.first, i.second));

			std::sort(newTemps.begin(), newTemps.end(), [](const std::pair< int, int > &a, const std::pair< int, int > &b)
				{
					return a.first < b.first;
				});

			std::lock_guard< std::mutex > l{ _mutex };
			std::swap(newTemps, _mixTemps);
		}

	public:
		MixTempApprox(Config &cfg)
		{
			_updateFromCfg(cfg);
			cfg.addUpdateListener([this](const Config &v) { this->_updateFromCfg(v); });
		}

		int getMixTemp(const OutsideTemp &outside, std::string *log)
		{
			std::lock_guard< std::mutex > l{ _mutex };

			std::vector< std::pair< int, int > >::iterator 
				max = _mixTemps.begin(),
				min = _mixTemps.begin() + 1;

			while (min != _mixTemps.end() && min->first < (int) outside)
			{
				++min;
				++max;
			}

			if (min == _mixTemps.end())
				min = max--;
			
			*log = std::string("Mixer: using interval ") + std::to_string(max->first) + " to " + std::to_string(min->first);

			double x = (double) min->first - (int) outside;
			double xWidth = (double) min->first - max->first;
			int ret = (int) (min->second + x / xWidth * (max->second - min->second));

			*log += std::string("\nMixer: for outside=") + outside.toString() + " need mix=" + std::to_string(ret);

			return ret;
		}
	};

	class ValvePosition
	{
		int _valveTurnSec;
	public:
		ValvePosition(Gpio &gpio, const Config &cfg)
		{
			_valveTurnSec = cfg.valveTurnTimeSec;
			
			adjustBy(-1, gpio);
		}

		void adjustBy(double turn, Gpio &gpio)
		{
			int turnTime = (int) (fabs(turn) * _valveTurnSec);

			log(std::string("Mixer: rotate by ") + std::to_string(turn) + ", impulse len=" + std::to_string(turnTime) + " sec");

			Timer t;
			t.setSeconds(turnTime);

			if (turn > 0)
				gpio.tempValveHot();
			else
				gpio.tempValveCold();

			for (sleep(1); !t.expired() && gpio.isTempMotorOn(); sleep(1))
					;

			gpio.tempValveOff();
		}
	};

	class Mixer
	{
		std::unique_ptr< std::thread > _thread;
		std::atomic_bool _needsHeat, _quit;
		std::atomic_int _insideExpectedTemp, _insideActualTemp, _outsideTemp,
			_mixInHotTemp, _mixInColdTemp, _mixResultTemp;

		enum class MixPos : int
		{
			Max,
			Min, 
			Med
		} _mixPos;

		struct MixSettings
		{
			int mixOut;
            OutsideTemp outsideTemp;
			std::string updateLog;

			MixSettings()
				: mixOut(0)
			{
			}

			MixSettings(const OutsideTemp &outside, const std::shared_ptr<MixTempApprox> &tempApprox)
				: mixOut(0), outsideTemp(outside)
			{
				mixOut = tempApprox->getMixTemp(outside, &updateLog);
			}

			bool operator ==(const MixSettings &other) const
			{
				return outsideTemp.effective == other.outsideTemp.effective
					&& mixOut == other.mixOut;
			}
		} _mixSettings;

		ValvePosition _valvePos;
		constexpr static int checkIntervalSec = 5;
		constexpr static size_t minCyclesBeforePumpOff = 2 * 60 / checkIntervalSec;
		constexpr static size_t cyclesForInstantPumpOff = 1;
		size_t _cyclesToPumpOff;

		void _mixerThread(Gpio &gpio, const std::shared_ptr<MixTempApprox> &tempApprox)
		{
			for (; !_quit; sleep(checkIntervalSec))
			{
				MixSettings current(
                    OutsideTemp(_outsideTemp, _insideActualTemp - _insideExpectedTemp),
                    tempApprox);
				
				if (!(current == _mixSettings))
				{
					log(current.updateLog);
					_mixSettings = current;
				}

				_needsHeat = false;

				if (!roomsNeedHeat())
				{
					_maxColdFlow(gpio);
				}
				else if (_mixSettings.mixOut >= _mixInHotTemp)
				{
					_needsHeat = true;
					_maxHotFlow(gpio);
				}
				else
					_mixSetAndCheck(_mixSettings.mixOut, gpio);
			}
		}

		void _maxHotFlow(Gpio &gpio)
		{
			if (_mixPos != MixPos::Max)
			{
				gpio.radiatorPumpOn();
				_cyclesToPumpOff = minCyclesBeforePumpOff;
				_valvePos.adjustBy(1, gpio);
			}

			_mixPos = MixPos::Max;
		}

		void _maxColdFlow(Gpio &gpio)
		{
			if (_mixPos != MixPos::Min)
				_valvePos.adjustBy(-1, gpio);
			
			_mixPos = MixPos::Min;

			if (!--_cyclesToPumpOff)
				gpio.radiatorPumpOff();
		}

		void _mixSetAndCheck(int requiredMixTemp, Gpio &gpio)
		{
			gpio.radiatorPumpOn();
			_cyclesToPumpOff = minCyclesBeforePumpOff;

			_mixPos = MixPos::Med;

			const int delta = 2;
			const int diff = requiredMixTemp - _mixResultTemp;
			if (abs(diff) <= delta)
				return;

			double totalWidth = _mixInHotTemp - _mixInColdTemp;
			if (fabs(totalWidth) < 1)
				totalWidth = 1;

			double halfTurnATime = (double) diff / 2;

			_valvePos.adjustBy(std::clamp(halfTurnATime / totalWidth, -1.0, 1.0), gpio);
		}

	public:
		Mixer(Config &cfg, Gpio &gpio)
			: _needsHeat(false), _quit(false),
			_insideExpectedTemp(21), _insideActualTemp(21), _outsideTemp(21),
			_mixInHotTemp(21), _mixInColdTemp(21), _mixResultTemp(21),
			_mixPos(MixPos::Med),
			_valvePos(gpio, cfg),
			_cyclesToPumpOff(cyclesForInstantPumpOff)
		{
			std::shared_ptr< MixTempApprox > tempApprox(new MixTempApprox(cfg));

			_thread.reset(new std::thread([this, &gpio, tempApprox]()
			{
				this->_mixerThread(gpio, tempApprox);
			}));
		}

		~Mixer()
		{
			quit();
		}

		void quit()
		{
			if (!_quit)
			{
				_quit = true;
				_thread->join();
			}
		}

		bool roomsNeedHeat()
		{
			return _mixSettings.mixOut > _insideExpectedTemp;
		}

		bool needsHeat() const
		{
			return _needsHeat;
		}

		void setTemp(int expected, int actual, int outside, int mixCold, int mixHot, int mixActual)
		{
			_insideExpectedTemp = expected;
			_insideActualTemp = actual;
			_outsideTemp = outside;

			_mixInHotTemp = mixHot;
			_mixInColdTemp = mixCold;
			_mixResultTemp = mixActual;
		}
	};

}
