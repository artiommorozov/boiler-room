#ifndef ds18b20_h_included
#define ds18b20_h_included

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

		unsigned char low = port.readByte();
		unsigned char high = port.readByte();

		int ret = (low >> 4) + ((high & 7) << 4);

		if (high & ~7)
			ret = -ret;

		return ret;
	}

	template< typename Port >
	int convertAndReadTemp(Port &port, const OneWire::Rom &id)
	{
		port.matchRom(id);
		port.sendByte(Ds18b20::Cmds::convert);
		port.waitReadOne();

		port.reset();
		return readTempFromScratchpad(port, id);
	}
}

#endif