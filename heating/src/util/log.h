#pragma once

#include <time.h>
#include <sstream>
#include <fstream>
#include "config/Config.h"
#include <mutex>

namespace Logging
{
	typedef std::basic_ostream< char, std::char_traits< char > > Ostream;

	enum class Dst
	{
		Temp,
		Control
	};

	static std::mutex logMutex;
	static std::shared_ptr<Ostream> control{ new std::stringstream };
	static std::shared_ptr<Ostream> temp{ new std::stringstream };

	static std::string controlDir, tempDir, rotateCmd;

	void updateLogFiles(bool force = false)
	{
		static int lastDay = -1;

		struct tm now;
		time_t sec;
		time(&sec);
		localtime_r(&sec, &now);
		if (force || ((now.tm_mday != lastDay) && controlDir.length() && tempDir.length()))
		{
			lastDay = now.tm_mday;

			std::string filename(std::string("/day") + std::to_string(lastDay) + ".txt");

			{
				std::lock_guard< std::mutex > l{ logMutex };
				control.reset(new std::ofstream(controlDir + filename, std::ios::app));
				temp.reset(new std::ofstream(tempDir + filename, std::ios::app));
			}

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

		if (dst == Dst::Temp)
		{
			std::lock_guard< std::mutex > l{ logMutex };
			(*temp) << ((const char*)buf) << "," << msg << "\n";
			temp->flush();
		}
		else 
		{
			(*control) << ((const char*)buf) << ": " << msg << "\n";
			control->flush();
		}
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