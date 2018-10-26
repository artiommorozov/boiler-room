#include "tempSensor/TempSensor.h"
#include "tempSensor/Ds18b20.h"
#include "tempSensor/Ds18s20.h"
#include "tempSensor/Ds2482.h"
#include "tempSensor/1wire.h"
#include "util/WaitCycle.h"
#include "util/dbglog.h"
#include <map>
#include <algorithm>
#include <iostream>

using namespace Temp;

typedef PeriodicCheckWait< > WaitCycle;
typedef Ds2482::ChipInterface< WaitCycle > Interface;
typedef Ds2482::Port< Interface > Port;

struct LineBulkQuery
{
	std::vector< std::shared_ptr< ISensor > > sensors;
	Port *port;
};

class Sensor : public ISensor, Noncopyable
{
protected:
	OneWire::Rom _id;
	Port _port;
	
	static std::map<Ds2482::AdapterPort, LineBulkQuery> groupByLine(const std::vector< std::shared_ptr< ISensor > > &sensors)
	{
		std::map<Ds2482::AdapterPort, LineBulkQuery> perLine;
		for (auto &i : sensors)
		{
			Sensor *s = (Sensor*)i.get();
			auto &group = perLine[s->_port.index()];
			group.sensors.push_back(i);
			group.port = &s->_port;
		}

		return perLine;
	}

	static std::map< OneWire::Rom, int > readAllWhenReady(std::map<Ds2482::AdapterPort, LineBulkQuery> &perLine)
	{
		std::map< OneWire::Rom, int > readings;
		for (auto &lineSensors : perLine)
		{
			Port &port = *lineSensors.second.port;
			OneWire::session(port, std::string("waiting for conversion to finish"), false)
				.perform([](Port &p) { p.waitReadOne(); });

			for (auto &sensor : lineSensors.second.sensors)
			{
				const OneWire::Rom &id = sensor->id();
				OneWire::session(port, std::string("reading temp value of ") + sensor->id().toString())
					.perform([&](Port &p) {
						readings[id] = ((Sensor*)sensor.get())->_readScratchpad(p, id);
				});
			}
		}

		return readings;
	}

	std::function< int(Port &, const OneWire::Rom &) > _readScratchpad;

public:
	Sensor(const std::shared_ptr< Interface > &chipInterface, Ds2482::AdapterPort line, const OneWire::Rom &id)
		: _id(id), _port(chipInterface, line),
		_readScratchpad(Ds18b20::readTempFromScratchpad<Port>)
	{
		Ds1820Type type;
		OneWire::session(_port, "checking sensor type " + _id.toString())
			.perform(
				[&type, this](Port &port) { type = Ds18s20::type(port, this->_id); });

		switch (type)
		{
		case Ds1820Type::SType: _readScratchpad = Ds18s20::readTempFromScratchpad<Port>; break;

		case Ds1820Type::BType: break;

		case Ds1820Type::Unknown:
		default:
		 throw std::runtime_error(std::string("Unknown sensor type for id=") + id.toString());
		}
	}

	int read()
	{
		int ret;
		OneWire::session(_port, "reading temp from " + _id.toString())
			.perform(
				[&ret, this](Port &port) { ret = Ds18b20::convertAndReadTemp(port, this->_id, this->_readScratchpad); });

		return ret;
	}

	OneWire::Rom id() const
	{
		return _id;
	}

	static std::vector< int > readAll(
		const std::vector< std::shared_ptr< ISensor > > &sensors)
	{
		std::map<Ds2482::AdapterPort, LineBulkQuery> perLine = groupByLine(sensors);

		for (auto &lineSensors : perLine)
			OneWire::session(*lineSensors.second.port, "starting conversion")
			.perform([](Port &p) { Ds18b20::bulkConvert(p); });
		
		std::map< OneWire::Rom, int > readings = readAllWhenReady(perLine);

		std::vector<int> ret;
		std::transform(sensors.begin(), sensors.end(), std::back_inserter(ret),
			[&readings](const std::shared_ptr< ISensor > &sensor) { return readings[sensor->id()]; });

		return ret;
	}
};

namespace Temp
{
	std::vector< std::shared_ptr< ISensor > > openAllDs2482_800(const std::string &i2c_device, int i2c_address)
	{
		std::shared_ptr< Interface > ds2482(new Interface(i2c_device, i2c_address));
		std::vector< std::shared_ptr< ISensor > > ret;

		for (auto line : Ds2482::ds2482_800Ports)
		try
		{
			std::vector< OneWire::Rom > lineIds;
			Port port(ds2482, line);

			OneWire::session(port, "searching ROMs on line " + std::to_string((int)line))
				.perform(
					[&](Port &p)
					{
						for (auto &id : p.searchRom())
						{
						//	std::cout << " on line " << line << " found id " << id.toString() << "\n";
							ret.push_back(std::shared_ptr<ISensor>(new Sensor(ds2482, line, id)));
						}
					});
		}
		catch (const std::exception &e)
		{
			dbglog(e.what());
		}

		return ret;
	}

	std::vector< int > bulkRead(const std::vector< std::shared_ptr< ISensor > > &src)
	{
		return Sensor::readAll(src);
	}
}