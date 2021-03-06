// testcpp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/date_time.hpp>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <thread>
#include <mutex>
#include <map>

#include "util/clamp.h"

#ifdef _WIN32
#define GPIO_STUB

#include "windows.h"
void sleep(int x)
{
	Sleep(x * 1000);
}

void usleep(int x)
{
	Sleep(x / 1000);
}

#define CLOCK_MONOTONIC_RAW nullptr
#define CLOCK_MONOTONIC nullptr

void clock_gettime(int *x, struct timespec *t)
{
	t->tv_sec = time(NULL);
}

namespace Temp
{
	struct Id 
	{
		std::string id;
		std::string toString() const { return id; } 
	};
	struct ISensor
	{
		Id _id;
		explicit ISensor(const std::string &id)
			: _id{ id }
		{
		}

		int read()
		{
			return 0;
		}

		Id id()
		{
			return _id;
		}
	};

	//static std::vector< int > bulkReadRet{1,2,3,4,5,6,7,8,9,10};

	std::vector<int> bulkRead(const std::vector<std::shared_ptr<ISensor>> &)
	{
		std::ifstream f("temp.in");

		std::vector<int> ret(10, 0);
		for (auto &i : ret)
		{
			f >> i;
		}

		return ret;
	}

	std::vector<std::shared_ptr<ISensor>> openAllDs2482_800(const std::string&, int)
	{
		return std::vector<std::shared_ptr<ISensor>>{
			std::make_shared<ISensor>("idroom"),
			std::make_shared<ISensor>("idfurnoutA"),
			std::make_shared<ISensor>("idfurnoutB"),
			std::make_shared<ISensor>("idfurnret"),
			std::make_shared<ISensor>("idmixout"),
			std::make_shared<ISensor>("idmixres"),
			std::make_shared<ISensor>("idmixret"),
			std::make_shared<ISensor>("idres"),
			std::make_shared<ISensor>("idradreta"),
			std::make_shared<ISensor>("idradretb")
		};
	}
}
#endif

#include "main.cpp"