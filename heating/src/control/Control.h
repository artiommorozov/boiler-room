#pragma once

#include "Mixer.h"

namespace Heat
{
	class Control
	{
		enum class State
		{
			ResHot,
			ResCold,
			ResColdStartPump,
			ResHeating,
			ResDumpingExtraHeat,
			BoilerStart,
			BoilerStartPump,
			BoilerHeating,
			BoilerStop
		} _state;

		Timer _valveTimer, _pumpTimer, _boilerHeat;
		Mixer _mixer;
		Circulation _circ;

		void _furnaceStop(Gpio &gpio, const Config &cfg)
		{
			log("Control: furnace stop");

			gpio.furnaceOff();
			_pumpTimer.setMinutes(cfg.furnacePumpRunoutMin);
			_valveTimer.setMinutes(cfg.furnacePumpRunoutMin + 1);
		}

		void _tick(Gpio &gpio, Config &cfg, Temperature &temp, UserParams &user)
		{
			switch (_state)
			{
			case State::BoilerStart:
			{
				log("Control: boiler start");

				gpio.boilerValveOpen();
				gpio.furnaceValveOpen();

				_state = State::BoilerStartPump;
			} break;

			case State::BoilerStartPump:
			{
				log("Control: boiler start pump");

				gpio.furnacePumpOn();

				gpio.closeReservoirLine(cfg);

				gpio.furnaceOn();

				_state = State::BoilerHeating;

				log("Control: to boiler heating");
			} break;

			case State::BoilerHeating:
			{
				if (!gpio.boilerNeedsHeat())
				{
					log("Control: boiler hot");

					_furnaceStop(gpio, cfg);
					_state = State::BoilerStop;

					log("Control: boiler heating complete");
				}
			} break;

			case State::BoilerStop:
			{
				if (_pumpTimer.expired())
					gpio.furnacePumpOff();

				if (_valveTimer.expired())
				{
					gpio.furnaceValveClose();
					gpio.boilerValveClose();

					_state = State::ResHot;

					log("Control: from boiler to reservoir hot state");
				}
			} break;

			case State::ResHot:
			{
				if (_mixer.needsHeat())
				{
					_state = State::ResCold;
				}
				else if (gpio.boilerNeedsHeat())
				{
					if (!_boilerHeat.isSet())
					{
						_boilerHeat.setMinutes(cfg.delayBeforeBoilerHeatMin);
						log("Control: set boiler heating timeout");
					}
				
					if (_boilerHeat.expired())
						_state = State::BoilerStart;
					
				}
			} break;

			case State::ResCold:
			{
				log("Control: reservoir heating ready");

				gpio.furnaceValveOpen();

				_state = State::ResColdStartPump;

			} break;

			case State::ResColdStartPump:
			{
				log("Control: reservoir heating start");

				gpio.furnacePumpOn();

				gpio.openReservoirLine(cfg);

				gpio.furnaceOn();

				_state = State::ResHeating;
			}

			case State::ResHeating:
			{
				if (temp.reservoirHot())
				{
					log("Control: dump extra heat to reservoir");
					_state = State::ResDumpingExtraHeat;

					_furnaceStop(gpio, cfg);
				}
			} break;

			case State::ResDumpingExtraHeat:
			{
				gpio.furnaceOff();

				if (!temp.heatFlowsFromFurnaceToReservoir())
				{
					if (_pumpTimer.expired())
						gpio.furnacePumpOff();

					if (_valveTimer.expired())
					{
						log("Control: reservoir hot");

						gpio.furnaceValveClose();
						gpio.closeReservoirLine(cfg);

						_state = State::ResHot;
					}
				}
			} break;

			default:
				throw std::runtime_error("unexpected state");
			}
		}

		void _safetyCheck(Gpio &gpio, Config &cfg, Temperature &temp)
		{
			if (temp.furnaceHot())
			{
				gpio.furnaceOff();
				throw std::runtime_error("Furnace too hot");
			}

			if (gpio.isFurnaceOn() && !temp.circulationGood())
			{
				gpio.furnaceOff();
				throw std::runtime_error("Circulation failure detected");
			}
		}

	public:

		Control(Gpio &gpio, Config &cfg)
			: _state(State::ResHot), _mixer(cfg, gpio)
		{
		}

		void tick(Gpio &gpio, Config &cfg, Temperature &temp, UserParams &user)
		{
			_safetyCheck(gpio, cfg, temp);

			_tick(gpio, cfg, temp, user);

			_safetyCheck(gpio, cfg, temp);

			_circ.tick(gpio, cfg);

			_mixer.setTemp(user.requiredTempInside, temp.inside(), user.externalTemp,
				temp.mixCold(), temp.mixHot(), temp.mixActual());
		}
	};
}