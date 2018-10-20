#pragma once

#include <time.h>
#include <sstream>
#include <fstream>
#include "config/Config.h"

namespace Logging
{
	typedef std::basic_ostream< char, std::char_traits< char > > Ostream;

	enum class Dst
	{
		Temp,
		Control
	};

	static std::shared_ptr<Ostream> control;
	static std::shared_ptr<Ostream> temp;

	static std::string controlDir, tempDir, rotateCmd;

	void initFrom(Config &config)
	{
		auto updateFunc = [](const Config &cfg)
		{
			controlDir = cfg.controlDir;
			tempDir = cfg.tempDir;
			rotateCmd = cfg.rotateCmd;
		};

		updateFunc(config);
		config.addUpdateListener(updateFunc);
	}

	void write(const std::string &msg, Dst dst)
	{
		static int lastDay = -1;

		struct tm now;
		time_t sec;

		time(&sec);
		localtime_r(&sec, &now);
		if (now.tm_mday != lastDay)
		{
			lastDay = now.tm_mday;

			std::string filename(std::string("/day") + std::to_string(lastDay) + ".txt");
			control.reset(new std::ofstream(controlDir + filename, std::ios::ate));
			temp.reset(new std::ofstream(tempDir + filename, std::ios::ate));
		}

		char buf[0xff];
		ctime_r(&sec, buf);

		std::stringstream out;
		out << ((const char*)buf) << ": " << msg << "\n";

		if (dst == Dst::Temp)
			(*temp) << out.str();
		else 
			(*control) << out.str();
	}
}

void logTemp(const std::string &msg)
{
	Logging::write(msg, Logging::Dst::Temp);
}

void log(const std::string &msg)
{
	Logging::write(msg, Logging::Dst::Control);
}