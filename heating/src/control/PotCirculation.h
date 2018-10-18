#pragma once

namespace Heat
{
	class Circulation
	{
		enum class State
		{
			Off,
			On
		} _state;

		Timer _timer;

	public:

		Circulation()
			: _state(State::Off)
		{

		}

		void tick(Gpio &gpio, Config &cfg)
		{
			switch (_state)
			{
			case State::On:
			{
				if (_timer.expired())
				{
					_timer.setMinutes(cfg.circulationPumpIdleMin);
					_state = State::Off;

					gpio.circulationPumpOff();
				}
			} break;

			case State::Off:
			{
				if (!_timer.isSet() || _timer.expired())
				{
					_timer.setMinutes(cfg.circulationPumpRunMin);
					_state = State::On;

					gpio.circulationPumpOn();
				}
			} break;
			}
		}
	};
}