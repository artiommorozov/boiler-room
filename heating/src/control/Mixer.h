#pragma once

namespace Heat
{
	class MixTempApprox
	{
		std::mutex _mutex;
		std::vector< std::pair< int, int > > _mixTemps;

		void _updateFromCfg(const Config &cfg)
		{
			std::vector< std::pair< int, int > > newTemps;

			for (const auto &i : cfg.mixTemp)
				newTemps.push_back(std::make_pair(i.first, i.second));

			std::sort(newTemps.begin(), newTemps.end(), [](auto &a, auto &b)
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

		int getMixTemp(int outside, std::string *log)
		{
			std::lock_guard< std::mutex > l{ _mutex };

			std::vector< std::pair< int, int > >::iterator 
				max = _mixTemps.begin(),
				min = _mixTemps.begin() + 1;

			while (min != _mixTemps.end() && min->first < outside)
			{
				++min;
				++max;
			}

			if (min == _mixTemps.end())
				min = max--;
			
			*log = std::string("Mixer: using interval ") + std::to_string(max->first) + " to " + std::to_string(min->first);

			double x = (double) min->first - outside;
			double xWidth = (double) min->first - max->first;
			int ret = (int) (min->second + x / xWidth * (max->second - min->second));

			*log += std::string("\nMixer: for outside=") + std::to_string(outside) + " need mix=" + std::to_string(ret);

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
		std::atomic_bool _needsHeat = false, _quit = false;
		std::atomic_int _insideExpectedTemp = 21, _insideActualTemp = 21, _outsideTemp = 21,
			_mixInHotTemp = 0, _mixInColdTemp = 0, _mixResultTemp = 0;

		enum class MixPos : int
		{
			Max,
			Min, 
			Med
		} _mixPos = MixPos::Med;

		struct MixSettings
		{
			int outsideTemp = 0;
			int mixOut = 0;
			std::string updateLog{ "" };

			MixSettings()
			{
			}

			MixSettings(int outside, const std::shared_ptr<MixTempApprox> &tempApprox)
				: outsideTemp(outside)
			{
				mixOut = tempApprox->getMixTemp(outside, &updateLog);
			}

			bool operator ==(const MixSettings &other) const
			{
				return outsideTemp == other.outsideTemp
					&& mixOut == other.mixOut;
			}
		} _mixSettings;

		ValvePosition _valvePos;

		int _effectiveOutsideTemp()
		{
			// TODO: to config?
			const int innerDeltaCorrection = 5;

			int usingOutsideTemp = _outsideTemp;
			if (_insideActualTemp < _insideExpectedTemp)
				usingOutsideTemp -= innerDeltaCorrection;
			else if (_insideActualTemp > _insideExpectedTemp)
				usingOutsideTemp += innerDeltaCorrection;

			return usingOutsideTemp;
		}

		void _mixerThread(Gpio &gpio, const std::shared_ptr<MixTempApprox> &tempApprox)
		{
			const int checkIntervalSec = 5;

			for (; !_quit; sleep(checkIntervalSec))
			{
				MixSettings current{ _effectiveOutsideTemp(), tempApprox };
				
				if (!(current == _mixSettings))
				{
					log(current.updateLog);
					_mixSettings = current;
				}

				if (_mixSettings.mixOut >= _mixInHotTemp)
				{
					_needsHeat = true;
					_maxHotFlow(gpio);
					continue;
				}

				_needsHeat = false;

				if (_mixSettings.mixOut <= _insideExpectedTemp)
					_maxColdFlow(gpio);
				else
					_mixSetAndCheck(_mixSettings.mixOut, gpio);
			}
		}

		void _maxHotFlow(Gpio &gpio)
		{
			if (_mixPos != MixPos::Max)
			{
				gpio.radiatorPumpOn();
				_valvePos.adjustBy(1, gpio);
			}

			_mixPos = MixPos::Max;
		}

		void _maxColdFlow(Gpio &gpio)
		{
			if (_mixPos != MixPos::Min)
			{
				gpio.radiatorPumpOff();
				_valvePos.adjustBy(-1, gpio);
			}

			_mixPos = MixPos::Min;
		}

		void _mixSetAndCheck(int requiredMixTemp, Gpio &gpio)
		{
			gpio.radiatorPumpOn();

			_mixPos = MixPos::Med;

			const int delta = 2;
			const int diff = requiredMixTemp - _mixResultTemp;
			if (abs(diff) <= delta)
				return;

			double totalWidth = _mixInHotTemp - _mixInColdTemp;
			if (fabs(totalWidth) < 1)
				totalWidth = 1;

			double halfTurnATime = (double) diff / 2;

			_valvePos.adjustBy(std::clamp(diff / totalWidth, -1.0, 1.0), gpio);
		}

	public:
		Mixer(Config &cfg, Gpio &gpio)
			: _valvePos(gpio, cfg)
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
