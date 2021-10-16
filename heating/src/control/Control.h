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

				_timers.valveTurn.setSeconds(cfg.valveTurnTimeSec);
			} break;

			case State::Noop:
			{
				log("Control: idle");

				gpio.closeReservoirLineEnd();
				gpio.pumpValveClose();
			} break;

			case State::BoilerSoftHeating:
			{
				log("Control: boiler soft heating");

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

				gpio.dieselOff();
				gpio.electricHeaterOff();

				_timers.pump.setMinutes(cfg.furnacePumpRunoutMin);
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

				gpio.dieselOff();
				gpio.electricHeaterOff();

				_timers.pump.setMinutes(cfg.furnacePumpRunoutMin);
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
					_enterState(State::Noop);
			} break;

			case State::Noop:
			{
				if (gpio.boilerNeedsHeat())
					_enterState(State::BoilerSoftHeating)

				else if (_mixer.roomsNeedHeat() && !temp.reservoirHot())
					_enterState(State::ResSoftOpenLine);
			} break;

			case State::BoilerSoftHeating:
			{
				if (_timers.boiler.expired())
					_enterState(State::BoilerHardHeating);

				else if (_mixer.needsHeat())
					_enterState(State::BoilerHardHeating);

				else if (!gpio.boilerNeedsHeat())
					_enterState(State::BoilerFurnaceRundown);

			} break;

			case State::BoilerHardHeating:
			{
				if (!gpio.boilerNeedsHeat())
					_enterState(State::BoilerFurnaceRundown);
			} break;

			case State::BoilerFurnaceRundown:
			{
				if (_timers.pump.expired())
					_enterState(State::BoilerPumpRundown);
			} break;

			case State::ResPumpRundown:
			case State::BoilerPumpRundown:
			{
				if (_timers.valveTurn.expired())
					_enterState(State::Noop);
			} break;

			case State::ResSoftOpenLine:
			{
				if (_timers.valveTurn.expired())
					_enterState(State::ResSoftHeating);
			} break;

			case State::ResSoftHeating:
			{
				if (_mixer.needsHeat())
					_enterState(State::ResHardHeating);

				else if (gpio.boilerNeedsHeat())
					_enterState(State::ResHardHeating);

				else if (temp.reservoirHot())
					_enterState(State::ResFurnaceRundown);
			} break;

			case State::ResHardHeating:
			{
				if (temp.reservoirHot())
					_enterState(State::ResFurnaceRundown);
			} break;

			case State::ResFurnaceRundown:
			{
				if (_timers.pump.expired())
					_enterState(State::ResPumpRundown);
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
				constexpr int checksPerMinute = 24, secPerMinute = 60;
				if (++_circulationChecksFailed >= (cfg.allowCirculationDiffSec * checksPerMinute) / secPerMinute)
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

			_safetyCheck(gpio, cfg, temp);

			_circ.tick(gpio, cfg);

			_mixer.setTemp(user.requiredTempInside, temp.inside(), user.externalTemp,
				temp.mixCold(), temp.mixHot(), temp.mixActual());
		}
	};
}