#pragma once

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem.hpp>

#ifndef _WIN32
#define override
#endif

struct JsonConfig
{
	std::string _filename;
	time_t	_lastWrite;

	explicit JsonConfig(const std::string &filename)
		: _filename(filename)
	{
	}

	virtual void loadFromPropertyTree(boost::property_tree::ptree &tree) = 0;

	bool refresh()
	{
		using namespace boost::property_tree;

		try
		{
			std::time_t lastWrite = boost::filesystem::last_write_time(boost::filesystem::path(_filename));
			if (lastWrite == _lastWrite)
				return false;

			ptree tree;
			std::ifstream file(_filename.c_str());
			read_json(file, tree);

			loadFromPropertyTree(tree);

			_lastWrite = lastWrite;
			return true;
		}
		catch (const std::exception &e)
		{
			throw std::runtime_error(std::string("Failed to read json config ") + _filename + ", err=" + e.what());
		}

		return false;
	}
};

void log(const std::string&);

struct UserParams : public JsonConfig
{
	int requiredTempInside;
	int externalTemp;

	explicit UserParams(const std::string &filename)
		: JsonConfig(filename), requiredTempInside(22), externalTemp(22)
	{
		refresh();
	}

	void loadFromPropertyTree(boost::property_tree::ptree &tree) override
	{
		log("loading new user settings");

		requiredTempInside = tree.get<int>("temp.inside");
		externalTemp = tree.get<int>("temp.outside");
	}

	UserParams &refresh()
	{
		JsonConfig::refresh();
		return *this;
	}
};

struct Config : public JsonConfig
{
	std::string roomTemp;

	std::string furnaceTempA;
	std::string furnaceTempB;
	std::string furnaceReturnTemp;

	std::string mixOutTemp;
	std::string mixReservoirTemp;
	std::string mixReturnTemp;

	std::string reservoirLowTemp;

	std::string radiatorReturnTempA;
	std::string radiatorReturnTempB;

	std::string gpioMotorTempUp;
	std::string gpioMotorTempDown;
	std::string gpioMotorCloseBoiler;
	std::string gpioMotorOpenBoiler;
	std::string gpioFurnace;
	std::string gpioFurnacePump;
	std::string gpioFurnaceValve;
	std::string gpioBoilerValve;
	std::string gpioCirculationPump;
	std::string gpioRadiatorPump;

	std::string gpioTempMotorSense;
	std::string gpioBoilerSense;

	std::string controlDir;
	std::string tempDir;
	std::string rotateCmd;

	std::vector<int> gpioExports;

	std::map< int, int > mixTemp;

	std::string ds2482Port;
	int ds2482Address;

	int furnaceMaxTemp;
	int furnaceMaxOutDiff;
	int furnaceMaxOutReturnDiff;
	int reservoirLowMaxTemp;

	int valveTurnTimeSec;

	int delayBeforeBoilerHeatMin;
	int furnacePumpRunoutMin;
	int circulationPumpIdleMin;
	int circulationPumpRunMin;
	int allowCirculationDiffSec;

	std::list< std::function< void(const Config &cfg) > > _updateListeners;

	void addUpdateListener(const std::function< void(const Config &cfg) > &f)
	{
		_updateListeners.push_back(f);
	}

	explicit Config(const std::string &filename)
		: JsonConfig(filename)
	{
		refresh();
	}

	void loadFromPropertyTree(boost::property_tree::ptree &tree) override
	{
		log("loading new configuration");

		ds2482Port = tree.get<std::string>("ds2482.port");
		ds2482Address = tree.get<int>("ds2482.address", 0);

		for (auto &i : tree.get_child("gpio.export"))
			gpioExports.push_back(i.second.get_value<int>());

		roomTemp = tree.get<std::string>("tempSensors.room");

		furnaceTempA = tree.get<std::string>("tempSensors.furnace.outA");
		furnaceTempB = tree.get<std::string>("tempSensors.furnace.outB");
		furnaceReturnTemp = tree.get<std::string>("tempSensors.furnace.return");

		mixOutTemp = tree.get<std::string>("tempSensors.mix.out");
		mixReservoirTemp = tree.get<std::string>("tempSensors.mix.reservoir");
		mixReturnTemp = tree.get<std::string>("tempSensors.mix.return");

		reservoirLowTemp = tree.get<std::string>("tempSensors.reservoir");

		radiatorReturnTempA = tree.get<std::string>("tempSensors.radiators.returnA");
		radiatorReturnTempB = tree.get<std::string>("tempSensors.radiators.returnB");

		furnaceMaxTemp = tree.get<int>("maxTemp.furnaceOut");
		furnaceMaxOutDiff = tree.get<int>("maxTemp.furnaceOutDiff");
		furnaceMaxOutReturnDiff = tree.get<int>("maxTemp.furnaceOutReturnDiff");
		reservoirLowMaxTemp = tree.get<int>("maxTemp.reservoirBottom");

		valveTurnTimeSec = tree.get<int>("valves.fullTurnSec");

		delayBeforeBoilerHeatMin = tree.get<int>("delay.boilerHeatMin");
		furnacePumpRunoutMin = tree.get<int>("delay.furnacePumpRunoutMin");
		circulationPumpIdleMin = tree.get<int>("delay.circulationPumpIdleMin");
		circulationPumpRunMin = tree.get<int>("delay.circulationPumpRunMin");
		allowCirculationDiffSec = tree.get<int>("delay.allowCirculationDiffSec");

		gpioMotorTempUp = tree.get<std::string>("gpio.motorTempUp");
		gpioMotorTempDown = tree.get<std::string>("gpio.motorTempDown");
		gpioMotorCloseBoiler = tree.get<std::string>("gpio.motorCloseBoiler");
		gpioMotorOpenBoiler = tree.get<std::string>("gpio.motorOpenBoiler");
		gpioFurnace = tree.get<std::string>("gpio.furnace");
		gpioFurnacePump = tree.get<std::string>("gpio.furnacePump");
		gpioFurnaceValve = tree.get<std::string>("gpio.furnaceValve");
		gpioBoilerValve = tree.get<std::string>("gpio.boilerValve");
		gpioCirculationPump = tree.get<std::string>("gpio.circulationPump");
		gpioRadiatorPump = tree.get<std::string>("gpio.radiatorPump");

		gpioTempMotorSense = tree.get<std::string>("gpio.tempMotorSense");
		gpioBoilerSense = tree.get<std::string>("gpio.boilerSense");

		controlDir = tree.get<std::string>("log.controlDir");
		tempDir = tree.get<std::string>("log.tempDir");
		rotateCmd = tree.get<std::string>("log.rotateCmd");

		auto &mixTempNode = tree.get_child("mixTemp");
		for (auto &i : mixTempNode)
			mixTemp[atoi(i.first.c_str())] = atoi(i.second.data().c_str());

		if (furnaceMaxTemp >= 80)
			throw std::runtime_error("Too high furnace temp in config file");
	}

	Config& refresh()
	{
		if (JsonConfig::refresh())
		{
			for (auto &i : _updateListeners)
				i(*this);
		}

		return *this;
	}
};