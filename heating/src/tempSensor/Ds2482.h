#pragma once

#include "util/Noncopyable.h"
#include "util/ErrnoException.h"
#include "util/dbglog.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

namespace Ds2482
{
	namespace Commands
	{
		const std::vector<unsigned char> reset{ 0xf0 };
		const std::vector<unsigned char> readPtrToCfgRegister{ 0xe1, 0xc3 };
		const std::vector<unsigned char> readPtrToDataRegister{ 0xe1, 0xe1 };
		const std::vector<unsigned char> readPtrToStatusRegister{ 0xe1, 0xf0 };
		const std::vector<unsigned char> cfgSetActivePullup{ 0xd2, 0xe1 };
		const std::vector<unsigned char> channelSelect{ 0xc3 };
		const std::vector<unsigned char> oneWireReset{ 0xb4 };
		const std::vector<unsigned char> oneWireWriteByte{ 0xa5 };
		const std::vector<unsigned char> oneWireReadByte{ 0x96 };
		const std::vector<unsigned char> oneWireTriplet1{ 0x78, 0xff };
		const std::vector<unsigned char> oneWireTriplet0{ 0x78, 0x00 };
	}

	namespace StatusRegister
	{
		const unsigned char OneWireBusy = 1,
			PresencePulseDetected = 2,
			ShortDetected = 4,
			LogicLevel = 8,
			DeviceReset = 0x10,
			SingleBitResult = 0x20,
			TripletSecondBit = 0x40,
			BranchDirectionTaken = 0x80;
	};

	namespace ConfigRegister
	{
		const unsigned char ActivePullup = 1,
			StrongPullup = 4,
			OneWideHiSpeed = 8;
	};

	template< typename WaitCycle >
	class ChipInterface : Noncopyable
	{
		int _file;
		int _address;

	public:

		explicit ChipInterface(const std::string &devName, int address)
			: _address(address)
		{
			_file = open(devName.c_str(), O_RDWR);
			if (_file == -1)
				throw ErrnoException(devName, errno);

			if (ioctl(_file, I2C_SLAVE, address) < 0)
				throw ErrnoException("address selection ioctl", errno);

			dbglog("OneWireAdapter: reset\n");
			write(Commands::reset);

			sleep(1);

			unsigned char statusOnReset = readByte();

			dbglog("OneWireAdapter: status=%02x\n", statusOnReset);

			if ((statusOnReset & (StatusRegister::OneWireBusy | StatusRegister::ShortDetected))
				|| !(statusOnReset & StatusRegister::DeviceReset))
			{

				throw std::runtime_error(std::string("device reset failed, status=") + std::to_string(statusOnReset));
			}

			dbglog("OneWireAdapter: configure\n");
			write(Commands::cfgSetActivePullup);

			unsigned char cfg = readByte();
			if (!(cfg & ConfigRegister::ActivePullup)
				|| (cfg & (ConfigRegister::OneWideHiSpeed | ConfigRegister::StrongPullup)))
			{

				throw std::runtime_error(std::string("device config failed, cfg=") + std::to_string(cfg));
			}

			dbglog("OneWireAdapter: ready\n");
		}

		~ChipInterface()
		{
			if (_file != -1)
				close(_file);
		}

		void write(const std::vector<unsigned char> &data)
		{
			if (::write(_file, &data[0], data.size()) < (int)data.size())
				throw ErrnoException("failed writing to device", errno);
		}

		std::vector< unsigned char > read(size_t bytes)
		{
			std::vector< unsigned char > ret(bytes, 0);

			int readRet;
			if ((readRet = ::read(_file, &ret[0], bytes)) != bytes)
				throw ErrnoException(std::string("failed reading from device, ret=") + std::to_string(readRet), errno);

			return ret;
		}

		unsigned char readByte()
		{
			return read(1)[0];
		}

		unsigned char getStatusRegister(bool setStatusPtr)
		{
			if (setStatusPtr)
				write(Commands::readPtrToStatusRegister);

			return readByte();
		}

		unsigned char waitReady(bool setStatusPtr = true)
		{
			unsigned char lastStatus;

			if (!WaitCycle().waitUntil([&lastStatus, this, setStatusPtr]()
			{
				lastStatus = this->getStatusRegister(setStatusPtr);
				return !(lastStatus & StatusRegister::OneWireBusy);
			}))
			{
				throw std::runtime_error(std::string("Timeout waiting for 1wire line to free, last status=") + std::to_string(lastStatus));
			}

			return lastStatus;
		}

		void channelSelect(unsigned char index, unsigned char req, unsigned char resp)
		{
			dbglog("OneWireAdapter: selecting channel %02X req=%02X ack=%02X\n", index, req, resp);
			waitReady();

			std::vector<unsigned char> cmd = Commands::channelSelect;
			cmd.push_back(req);

			write(cmd);

			unsigned char ack = readByte();
			if (ack != resp)
				throw std::runtime_error(std::string("Failed selecting channel=") + std::to_string(req) + " ack=" + std::to_string(ack));
		}

		void resetAndCheckPresence()
		{
			waitReady();

			write(Commands::oneWireReset);

			if (!(waitReady() & StatusRegister::PresencePulseDetected))
				throw std::runtime_error("Presence pulse not detected");
		}

		void waitReadOne()
		{
			if (!WaitCycle().waitUntil([this]()
			{
				return this->oneWireReadByte() == (unsigned char)0xff;
			}))
			{
				throw std::runtime_error(std::string("Timeout waiting for 1wire line to read 1"));
			}
		}

		unsigned char oneWireReadByte()
		{
			waitReady();

			write(Commands::oneWireReadByte);

			waitReady(false);

			write(Commands::readPtrToDataRegister);

			return readByte();
		}

		void oneWireWriteByte(unsigned char v)
		{
			waitReady();

			std::vector< unsigned char > cmd = Commands::oneWireWriteByte;
			cmd.push_back(v);

			write(cmd);

			waitReady();
		}

		unsigned char oneWireTriplet(unsigned char directionTo1)
		{
			waitReady();

			if (directionTo1)
				write(Commands::oneWireTriplet1);
			else
				write(Commands::oneWireTriplet0);

			return waitReady();
		}
	};

	enum AdapterPort
	{
		IO0 = 0,
		IO1,
		IO2,
		IO3,
		IO4,
		IO5,
		IO6,
		IO7
	};

	const std::vector< AdapterPort > ds2482_800Ports{ IO0, IO1, IO2, IO3, IO4, IO5, IO6, IO7 };

	const unsigned char requestChannelByte[] = { 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87 };
	const unsigned char responseChannelByte[] = { 0xb8, 0xb1, 0xaa, 0xa3, 0x9c, 0x95, 0x8e, 0x87 };

	template< typename Interface >
	class Port : Noncopyable
	{
		std::shared_ptr< Interface > _adapter;
		AdapterPort _index;

	public:
		explicit Port(const std::shared_ptr< Interface > &adapter, AdapterPort index)
			: _adapter(adapter), _index(index)
		{
		}

		AdapterPort index() const
		{
			return _index;
		}

		void connect()
		{
			_adapter->channelSelect(_index, requestChannelByte[_index], responseChannelByte[_index]);
		}

		void reset()
		{
			_adapter->resetAndCheckPresence();
		}

		void matchRom(const OneWire::Rom &rom)
		{
			_adapter->oneWireWriteByte(OneWire::Commands::matchRom);

			for (auto i : rom.value)
				_adapter->oneWireWriteByte(i);
		}

		void skipRom()
		{
			_adapter->oneWireWriteByte(OneWire::Commands::skipRom);
		}

		void waitReadOne()
		{
			_adapter->waitReadOne();
		}

		void sendByte(unsigned char i)
		{
			_adapter->oneWireWriteByte(i);
		}

		unsigned char readByte()
		{
			return _adapter->oneWireReadByte();
		}

		std::vector< OneWire::Rom > searchRom()
		{
			std::vector< OneWire::Rom > ret;

			for (int startFrom = -1, finished = 0, discrepancyFound = 1; !finished && discrepancyFound; )
			{
				OneWire::Rom current = { { 0 } };
				_adapter->oneWireWriteByte(OneWire::Commands::searchRom);
				discrepancyFound = 0;

				for (int i = 0; i < 64; ++i)
				{
					unsigned char status = _adapter->oneWireTriplet(i <= startFrom ? 1 : 0);
					auto bit1 = status & StatusRegister::SingleBitResult;
					auto bit2 = status & StatusRegister::TripletSecondBit;

					if (bit1 && bit2)
						finished = 1;

					else if (bit1)
						current.value[i / 8] |= 1 << (i % 8);

					else if (!bit2 && startFrom < i && !discrepancyFound)
					{
						startFrom = i;
						discrepancyFound = 1;
					}
				}

				if (!finished)
					ret.push_back(current);
			}

			return ret;
		}
	};


}

