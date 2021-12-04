#pragma once

#include "Mixer.h"

namespace Heat
{
	class Control
	{
		enum class State
		{
			Start,
			Noop,
			BoilerSoftHeating,

			BoilerHardHeating,

			BoilerFurnaceRundown,
			BoilerPumpRundown,

			ResSoftOpenLine,
			ResSoftHeating,
			ResSoftToBoilerSoft,

			ResHardHeating,

			ResFurnaceRundown,
			ResPumpRundown,

			None
		} _state;

		struct  
		{
			Timer pump;
			Timer boiler;
			Timer valveTurn;
		} _timers;

		Mixer _mixer;
		Circulation _circ;

		/*void _furnaceStop(Gpio &gpio, const Config &cfg)
		{
			log("Control: furnace stop");

			gpio.furnaceOff();
			_pumpTimer.setMinutes(cfg.furnacePumpRunoutMin);
			_valveTimer.setMinutes(cfg.furnacePumpRunoutMin + 1);
		}*/
		
		void _setPumpRunoutTimer(Gpio &gpio, Config &cfg)
		{
			_timers.pump.setMinutes(gpio.isDieselOn() ? cfg.hardPumpRunoutMin : cfg.softPumpRunoutMin);
		}

		void _enterState(Gpio &gpio, Config &cfg, State st)
		{
			switch (st)
			{
			case State::Start:
			{
				log("Control: start");

				gpio.closeReservoirLineBegin();
				gpio.electricHeaterOff();
				gpio.dieselOff();
				gpio.furnacePumpOff();
				gpio.pumpValveClose();
				gpio.boilerValveClose();
				gpio.radiatorPumpOff();
				gpio.circulationPumpOff();
				gpio.dieselValveClose();

				_timers.valveTurn.setSeconds(cfg.valveTurnTimeSec);
			} break;

			case State::Noop:
			{
				log("Control: idle");

				gpio.closeReservoirLineEnd();
				gpio.pumpValveClose();
				gpio.dieselValveClose();
				gpio.boilerValveClose();
			} break;

			case State::BoilerSoftHeating:
			{
				log("Control: boiler soft heating");

				gpio.closeReservoirLineEnd(); // when entering from res soft heat

				gpio.pumpValveOpen();
				gpio.boilerValveOpen();
				gpio.furnacePumpOn();
				gpio.electricHeaterOn();

				_timers.boiler.setMinutes(cfg.delayBeforeBoilerHeatMin);
			} break;

			case State::BoilerHardHeating:
			{
				log("Control: boiler full heating");

				gpio.dieselOn();
			} break;

			case State::BoilerFurnaceRundown:
			{
				log("Control: boiler hot, furnace stop");

				_setPumpRunoutTimer(gpio, cfg);

				gpio.dieselOff();
				gpio.electricHeaterOff();

			} break;

			case State::ResPumpRundown:
			case State::BoilerPumpRundown:
			{
				log("Control: pump stop");

				gpio.furnacePumpOff();
				gpio.closeReservoirLineBegin();

				_timers.valveTurn.setSeconds(cfg.valveTurnTimeSec);
			} break;

			case State::ResSoftOpenLine:
			{
				log("Control: radiators soft heat, opening line");

				gpio.pumpValveOpen();
				gpio.openReservoirLineBegin();

				_timers.valveTurn.setSeconds(cfg.valveTurnTimeSec);
			} break;

			case State::ResSoftHeating:
			{
				log("Control: radiators soft heat");

				gpio.openReservoirLineEnd();
				gpio.furnacePumpOn();
				gpio.electricHeaterOn();

				_timers.valveTurn.setSeconds(cfg.valveTurnTimeSec);
			} break;

			case State::ResHardHeating:
			{
				log("Control: radiators full heat");

				gpio.dieselOn();
			} break;

			case State::ResFurnaceRundown:
			{
				log("Control: reservoir hot, furnace rundown");

				_setPumpRunoutTimer(gpio, cfg);

				gpio.dieselOff();
				gpio.electricHeaterOff();

			} break;

			case State::ResSoftToBoilerSoft:
			{
				log("Control: switching to boiler soft heat");

				gpio.boilerValveOpen();
				gpio.closeReservoirLineBegin();

				_timers.valveTurn.setSeconds(cfg.valveTurnTimeSec);
			} break;

			default: 
			{
				log("Unknown state");
				throw std::runtime_error("unexpected state");
			} break;
			}

			_state = st;
		}

		void _tick(Gpio &gpio, Config &cfg, Temperature &temp, UserParams &user)
		{
			switch (_state)
			{
			case State::Start:
			{
				if (_timers.valveTurn.expired())
					_enterState(gpio, cfg, State::Noop);
			} break;

			case State::Noop:
			{
				if (gpio.boilerNeedsHeat())
					_enterState(gpio, cfg, State::BoilerSoftHeating);

				else if (_mixer.roomsNeedHeat() && temp.reservoirNeedsSoftHeat())
					_enterState(gpio, cfg, State::ResSoftOpenLine);
			} break;

			case State::BoilerSoftHeating:
			{
				if (_timers.boiler.expired())
					_enterState(gpio, cfg, State::BoilerHardHeating);

				else if (_mixer.needsHeat())
					_enterState(gpio, cfg, State::BoilerHardHeating);

				else if (!gpio.boilerNeedsHeat())
					_enterState(gpio, cfg, State::BoilerFurnaceRundown);

			} break;

			case State::BoilerHardHeating:
			{
				if (!gpio.boilerNeedsHeat())
					_enterState(gpio, cfg, State::BoilerFurnaceRundown);
			} break;

			case State::BoilerFurnaceRundown:
			{
				if (_timers.pump.expired())
					_enterState(gpio, cfg, State::BoilerPumpRundown);
			} break;

			case State::ResPumpRundown:
			case State::BoilerPumpRundown:
			{
				if (_timers.valveTurn.expired())
					_enterState(gpio, cfg, State::Noop);
			} break;

			case State::ResSoftOpenLine:
			{
				if (_timers.valveTurn.expired())
					_enterState(gpio, cfg, State::ResSoftHeating);
			} break;

			case State::ResSoftHeating:
			{
				if (_mixer.needsHeat())
					_enterState(gpio, cfg, State::ResHardHeating);

				else if (gpio.boilerNeedsHeat())
					_enterState(gpio, cfg, State::ResSoftToBoilerSoft);

				else if (temp.reservoirHotSoftMode())
					_enterState(gpio, cfg, State::ResFurnaceRundown);
			} break;

			case State::ResSoftToBoilerSoft:
			{
				if (_timers.valveTurn.expired())
					_enterState(gpio, cfg, State::BoilerSoftHeating);
			} break;

			case State::ResHardHeating:
			{
				if (temp.reservoirHotHardMode())
					_enterState(gpio, cfg, State::ResFurnaceRundown);
			} break;

			case State::ResFurnaceRundown:
			{
				if (_timers.pump.expired())
					_enterState(gpio, cfg, State::ResPumpRundown);
			} break;

			default:
				throw std::runtime_error("unexpected state");
			}
		}

		int _circulationChecksFailed;

		void _safetyCheck(Gpio &gpio, Config &cfg, Temperature &temp)
		{
			if (temp.furnaceHot())
			{
				gpio.dieselOff();
				gpio.electricHeaterOff();
				throw std::runtime_error("Output is too hot");
			}

			if (gpio.isDieselOn() && !temp.circulationGood())
			{
				if (++_circulationChecksFailed >= cfg.allowCirculationDiffSec)
				{
					gpio.dieselOff();
					throw std::runtime_error("Circulation failure detected");
				}
				else
					return;
			}

			_circulationChecksFailed = 0;
		}

	public:

		Control(Gpio &gpio, Config &cfg)
			: _state(State::None), _mixer(cfg, gpio), _circulationChecksFailed(0)
		{
			_enterState(gpio, cfg, State::Start);
		}

		void tick(Gpio &gpio, Config &cfg, Temperature &temp, UserParams &user)
		{
			_safetyCheck(gpio, cfg, temp);

			_tick(gpio, cfg, temp, user);

			_circ.tick(gpio, cfg);

			_mixer.setTemp(user.requiredTempInside, temp.inside(), user.externalTemp,
				temp.mixCold(), temp.mixHot(), temp.mixActual());
		}
	};
}