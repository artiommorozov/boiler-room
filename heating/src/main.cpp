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

int main(int argc, const char **argv)
{
	if (argc < 3)
	{
		printf("Use %s <config.json> <userCfg.json>\n", argv[0]);
		return -1;
	}

	try
	{
		Config cfg(argv[1]);
		Temperature temp(cfg);
		UserParams userParams(argv[2]);
		Gpio gpio(cfg);

		try
		{
			Control control{gpio, cfg};

			while (1)
			{
			//	temp.read();
			//	temp.dumpReadings();

				control.tick(gpio, cfg.refresh(), temp.read(), userParams.refresh());
				sleep(1);
			}
		}
		catch (const std::exception &e)
		{
			log(std::string("*** ERROR: ") + e.what());

			gpio.furnaceOff();
			gpio.furnaceValveOpen();
			gpio.furnacePumpOn();
			sleep(60);

			throw;
		}
	}
	catch (const std::exception& e)
	{
		printf("error: %s\n", e.what());
	}
	
    return 0;
}


