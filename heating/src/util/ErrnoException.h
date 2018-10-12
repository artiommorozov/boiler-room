#pragma once

#include <string>
#include <stdexcept>
#include <string.h>

class ErrnoException : public std::runtime_error
{

	static std::string _stdError(int e)
	{
		char buf[0x400];
		return strerror_r(e, buf, sizeof(buf));
	}

public:
	ErrnoException(const std::string &prefix, int e)
		: std::runtime_error(prefix + " err=" + _stdError(e))
	{

	}
};
