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

	void updateLogFiles(bool force = false)
	{
		static int lastDay = -1;

		struct tm now;
		time_t sec;
		time(&sec);
		localtime_r(&sec, &now);
		if (force || (now.tm_mday != lastDay && controlDir.length() && tempDir.length()))
		{
			lastDay = now.tm_mday;

			std::string filename(std::string("/day") + std::to_string(lastDay) + ".txt");
			control.reset(new std::ofstream(controlDir + filename, std::ios::ate));
			temp.reset(new std::ofstream(tempDir + filename, std::ios::ate));
			system(rotateCmd.c_str());
		}
	}

	void initFrom(Config &config)
	{
		auto updateFunc = [&](const Config &cfg)
		{
			controlDir = cfg.controlDir;
			tempDir = cfg.tempDir;
			rotateCmd = cfg.rotateCmd;

			updateLogFiles(true);
		};

		updateFunc(config);
		config.addUpdateListener(updateFunc);
	}

	void write(const std::string &msg, Dst dst)
	{
		updateLogFiles();

		time_t sec;
		time(&sec);

		char buf[0xff];
		ctime_r(&sec, buf);
		buf[strlen(buf) - 1] = 0;

		std::stringstream out;
		out << ((const char*)buf) << ": " << msg << "\n";

		if (dst == Dst::Temp)
		{
			if (temp)
				(*temp) << out.str();
		}
		else if (control)
			(*control) << out.str();

		std::cout << out.str();
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