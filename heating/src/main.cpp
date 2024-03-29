#ifndef TESTMODE

#include <stdio.h>
#include <iostream>
#include "tempSensor/TempSensor.h"
#include "util/Hexstr.h"

#endif

#include "util/log.h"

#include "config/Config.h"

#include "gpio/Gpio.h"

#include "util/Timer.h"

#include "control/Temperature.h"
#include "control/Mixer.h"
#include "control/PotCirculation.h"
#include "control/Control.h"

using namespace Heat;

int dumpTemp(const char *device, const char *port)
{
	try
	{
		auto sensors = Temp::openAllDs2482_800(device, strtol(port, nullptr, 16));
		for (auto &i : sensors)
		{
			const auto &id = i->id();
			std::cout << "sensor " << id.toString() << " reads " << i->read() << "*C\n";
		}

		while (1)
		{
			auto readings = Temp::bulkRead(sensors);
			time_t sec;
			time(&sec);
			char buf[0xff];
			ctime_r(&sec, buf);

			for (size_t k = 0; k < sensors.size(); ++k)
			{	
				std::cout << buf;
				std::cout << sensors[k]->id().toString() << " reads " << readings[k] << "*C\n";
			}
		}
	}
	catch (const std::exception &e)
	{
		std::cout << "*** ERROR: " << e.what() << "\n";
	}

	return 0;
}


int main(int argc, const char **argv)
{
	if (argc < 3)
	{
		printf("Use %s <config.json> <userCfg.json>\nor %s --temp ds2482_i2c_device ds2482_port_hex\n", argv[0], argv[0]);
		return -1;
	}

	if (argc > 3 && !strcmp(argv[1], "--temp"))
		return dumpTemp(argv[2], argv[3]);

	try
	{
		Config cfg(argv[1]);
		Logging::initFrom(cfg);

		Temperature temp(cfg);
		UserParams userParams(argv[2]);
		Gpio gpio(cfg);

		try
		{
			Control control{gpio, cfg};

			while (1)
			{
				control.tick(gpio, cfg.refresh(), temp.read(), userParams.refresh());
				sleep(1);
			}
		}
		catch (const std::exception &e)
		{
			log(std::string("*** ERROR: ") + e.what());

			gpio.electricHeaterOff();
			gpio.dieselOff();
			gpio.pumpValveOpen();
			gpio.boilerValveOpen();
			gpio.furnacePumpOn();
			sleep(60);

			throw;
		}
	}
	catch (const std::exception& e)
	{
		std::cout << "*** ERROR: " << e.what() << "\n";
	}
	
    return 0;
}


