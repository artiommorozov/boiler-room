#pragma once

namespace Heat
{
	class Circulation
	{
		enum class State
		{
			Off,
			On
		} _state = State::Off;

		Timer _timer;

	public:

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