#pragma once

#include "Ds18b20.h"

enum class Ds1820Type
{
	BType,
	SType,
	Unknown
};


namespace Ds18s20
{
	template< typename Port >
	int readTempFromScratchpad(Port &port, const OneWire::Rom &id)
	{
		port.matchRom(id);
		port.sendByte(Ds18b20::Cmds::readScratchpad);

		unsigned char low = port.readByte();
		unsigned char high = port.readByte();

		int ret = (low >> 1) + ((low & 1) ? 1 : 0);

		if (high)
			ret = -ret;

		return ret;
	}

	template< typename Port >
	Ds1820Type type(Port &port, const OneWire::Rom &id) 
	{
		port.matchRom(id);
		port.sendByte(Ds18b20::Cmds::readScratchpad);

		port.readByte();
		port.readByte();
		port.readByte();

		unsigned char reserved = port.readByte();
		if (reserved == (unsigned char) 0xff)	
			return Ds1820Type::SType;

		if ((reserved & 0x9f) == (unsigned char) 0x1f)
			return Ds1820Type::BType;

		return Ds1820Type::Unknown;
	}
}

