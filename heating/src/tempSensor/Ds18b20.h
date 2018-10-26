#pragma once

namespace Ds18b20
{
	namespace Cmds {
		const unsigned char readScratchpad = 0xbe,
			convert = 0x44;
	}

	template< typename Port >
	void bulkConvert(Port &port)
	{
		port.skipRom();
		port.sendByte(Ds18b20::Cmds::convert);
	}

	template< typename Port >
	int readTempFromScratchpad(Port &port, const OneWire::Rom &id)
	{
		port.matchRom(id);

		port.sendByte(Ds18b20::Cmds::readScratchpad);

		int ret;
		unsigned char low = port.readByte();
		unsigned char high = port.readByte();
		port.readByte();
		port.readByte();
		unsigned char cfg = port.readByte();		

		const int res9bit = 0, res10bit = 1, res11bit = 2, res12bit = 3;

		switch ((cfg >> 5) & 0x3)
		{
		case res9bit: ret = (low >> 1) + ((low & 1) ? 1 : 0); break;

		case res10bit: ret = (low >> 2) + ((high & 1) << 6) + ((low & 2) ? 1 : 0); break;

		case res11bit: ret = (low >> 3) + ((high & 3) << 5) + ((low & 4) ? 1 : 0); break;
		
		case res12bit: ret = (low >> 4) + ((high & 7) << 4) + ((low & 8) ? 1 : 0); break;
		}

		if (high & 0x80)
			ret = -ret;

		return ret;
	}

	template< typename Port, typename ReadScratchpad >
	int convertAndReadTemp(Port &port, const OneWire::Rom &id, ReadScratchpad &readScratchpad)
	{
		port.matchRom(id);
		port.sendByte(Ds18b20::Cmds::convert);
		port.waitReadOne();

		port.reset();
		return readScratchpad(port, id);
	}
}

