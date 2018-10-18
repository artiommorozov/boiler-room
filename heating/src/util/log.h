#pragma once

#include <time.h>
#include <sstream>

void log(const std::string &msg)
{
	time_t t = time(NULL);
	char buf[0xff];
	ctime_r(&t, buf);

	std::stringstream out;

	out << ((const char*) buf) << ": " << msg << "\n";
	std::cout << out.str();
}