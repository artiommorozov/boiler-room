#pragma once

#include "tempSensor/1wire.h"
#include <vector>
#include <string>
#include <memory>

namespace Temp
{

	struct ISensor 
	{
		virtual ~ISensor() {}

		virtual int read() = 0;

		virtual OneWire::Rom id() const = 0;
	};

	std::vector< std::shared_ptr< ISensor > > openAllDs2482_800(const std::string &i2c_device, int i2c_address);
	std::vector< int > bulkRead(const std::vector< std::shared_ptr< ISensor > > &src);
}

