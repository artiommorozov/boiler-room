#include <stdio.h>
#include <iostream>
#include "tempSensor/TempSensor.h"
#include "util/Hexstr.h"

int main(int argc, const char **argv)
{
	if (argc < 3)
	{
		printf("Use %s ds2482_i2c_device ds2482_address_hex\n", argv[0]);
		return -1;
	}

	for (auto i : Temp::openAllDs2482_800(argv[1], strtol(argv[2], nullptr, 16)))
	{
		auto id = i->id();
		std::cout << "sensor " << id.toString() << " reads=" << i->read() << "\n";
	}

	return 0;
}
