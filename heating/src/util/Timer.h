#pragma once

class Timer
{
	bool _set = false;
	struct timespec _time = { 0, 0 };

public:
	bool expired() 
	{
		if (!_set)
			return false;

		struct timespec t = { 0, 0 };
		clock_gettime(CLOCK_MONOTONIC, &t);

		if (t.tv_sec >= _time.tv_sec)
		{
			_set = false;
			return true;
		}

		return false;
	}

	void setMinutes(int min)
	{
		setSeconds(min * 60);
	}

	void setSeconds(int sec)
	{
		clock_gettime(CLOCK_MONOTONIC, &_time);
		_time.tv_sec += sec;
		_set = true;
	}

	bool isSet() const
	{
		return _set;
	}
};