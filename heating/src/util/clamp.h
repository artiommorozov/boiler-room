#pragma once

// skip on c++17

namespace std
{
	template<typename T>
	T clamp(const T &v, const T &min, const T &max)
	{
		if (v > max)
			return max;
		else if (v < min)
			return min;

		return v;
	}
}