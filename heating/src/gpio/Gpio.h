#pragma once

#include "boost/noncopyable.hpp"

static const char *gpioRoot = "/sys/class/gpio/";

class GpioPin : boost::noncopyable
{
	std::string _pinDir;

public:
	GpioPin(const std::string &pinDir, const std::string &dirValue)
		: _pinDir(pinDir)
	{
		{
			std::ofstream dir(std::string(pinDir) + "/direction");
			dir << dirValue << "\n";
		}

#ifndef GPIO_STUB
		{
			std::ifstream dir(std::string(pinDir) + "/direction");
			std::string v;
			std::getline(dir, v);
			if (v != dirValue)
				throw std::runtime_error(std::string("Failed setting direction on ") + pinDir + ", validation failed, read=" + v);
		}
#endif
	}

	const std::string &name() const
	{
		return _pinDir;
	}
};

class GpioInPin : GpioPin
{
	std::ifstream _f;

public:
	explicit GpioInPin(const std::string &pinDir)
		: GpioPin(pinDir, "in"),
		_f(pinDir + "/value")
	{
		if (!_f.good())
			throw std::runtime_error(pinDir + "/value: failed to read");
	}

	bool isLow()
	{
		// TODO: check on LINUX!
#ifdef GPIO_STUB
		_f.seekg(0);
#endif
		std::string v;
		std::getline(_f, v);
		if (v == "1")
			return false;
		else if (v == "0")
			return true;

		throw std::runtime_error(std::string("Reading from pin ") + name() + " produced invalid value=" + v);
	}
};

class GpioOutPin : GpioPin
{
	std::ofstream _f;
	bool _low;

public:
	explicit GpioOutPin(const std::string &pinDir)
		: GpioPin(pinDir, "out"),
		_f(pinDir + "/value")
	{
		low();
	}

	~GpioOutPin()
	{
		low();
	}

	bool low()
	{
		bool ret = !_low;

		_f << "0\n";
		_low = true;

		return ret;
	}

	bool high()
	{
		bool ret = _low;

		_f << "1\n";
		_low = false;

		return ret;
	}
};

class Gpio : boost::noncopyable
{
	void _export(int pin)
	{
		std::ofstream ctrl(std::string(gpioRoot) + "export");
		ctrl << pin << "\n";
	}

	std::unique_ptr< GpioOutPin > _motorTempUp, _motorTempDown, _motorCloseBoiler, _motorOpenBoiler, 
		_furnace, _furnacePump, _furnaceValve, _boilerValve, _circulationPump, _radiatorPump;

	std::unique_ptr< GpioInPin > _tempMotorSense, _boilerSense;

	bool _furnaceOn;
	bool _resLineClosed;

public:
	explicit Gpio(const Config &cfg)
		: _furnaceOn(false), _resLineClosed(false)
	{
		for (int x : cfg.gpioExports)
			_export(x);

		_motorTempUp.reset(new GpioOutPin(cfg.gpioMotorTempUp));
		_motorTempDown.reset(new GpioOutPin(cfg.gpioMotorTempDown));
		_motorCloseBoiler.reset(new GpioOutPin(cfg.gpioMotorCloseBoiler));
		_motorOpenBoiler.reset(new GpioOutPin(cfg.gpioMotorOpenBoiler));
		_furnace.reset(new GpioOutPin(cfg.gpioFurnace));
		_furnacePump.reset(new GpioOutPin(cfg.gpioFurnacePump));
		_furnaceValve.reset(new GpioOutPin(cfg.gpioFurnaceValve));
		_boilerValve.reset(new GpioOutPin(cfg.gpioBoilerValve));
		_circulationPump.reset(new GpioOutPin(cfg.gpioCirculationPump));
		_radiatorPump.reset(new GpioOutPin(cfg.gpioRadiatorPump));
		
		_tempMotorSense.reset(new GpioInPin(cfg.gpioTempMotorSense));
		_boilerSense.reset(new GpioInPin(cfg.gpioBoilerSense));

		log("GPIO init complete");

		closeReservoirLine(cfg);
	}

	void furnaceOff()
	{
		_furnace->low();

		if (!_furnaceOn)
		{
			_furnaceOn = false;
			log("furnace off");
		}
	}

	void furnaceOn()
	{
		_furnaceOn = true;
		_furnace->high();

		log("furnace ON");
	}

	bool isFurnaceOn() const
	{
		return _furnaceOn;
	}

	void furnacePumpOn()
	{
		_furnacePump->high();

		log("furnace pump ON");
	}

	void furnacePumpOff()
	{
		_furnacePump->low();

		log("furnace pump off");
	}

	void boilerValveOpen()
	{
		_boilerValve->high();
		log("boiler valve OPEN");
	}

	void boilerValveClose()
	{
		_boilerValve->low();
		log("boiler valve closed");
	}

	void furnaceValveOpen()
	{
		_furnaceValve->high();
		log("furnace valve OPEN");
	}

	void furnaceValveClose()
	{
		_furnaceValve->low();
		log("furnace valve closed");
	}

	bool boilerNeedsHeat()
	{
		return _boilerSense->isLow();
	}

	void circulationPumpOff()
	{
		_circulationPump->low();
		log("pot water circulation pump off");
	}

	void circulationPumpOn()
	{
		_circulationPump->high();
		log("pot water circulation pump ON");
	}

	void radiatorPumpOff()
	{
		if (_radiatorPump->low())
			log("radiator pump off");
	}

	void radiatorPumpOn()
	{
		if (_radiatorPump->high())
			log("radiator pump ON");
	}

	void closeReservoirLine(const Config &cfg)
	{
		if (_resLineClosed)
			return;

		log("reservoir line closing / boiler line open");

		_motorCloseBoiler->low();
		sleep(1);
		_motorOpenBoiler->high();
		sleep(cfg.valveTurnTimeSec);
		_motorOpenBoiler->low();

		log("reservoir line CLOSED / boiler line OPENED");

		_resLineClosed = true;
	}

	void openReservoirLine(const Config &cfg)
	{
		if (!_resLineClosed)
			return;

		log("reservoir line open/ boiler line closing");

		_motorOpenBoiler->low();
		sleep(1);
		_motorCloseBoiler->high();
		sleep(cfg.valveTurnTimeSec);
		_motorCloseBoiler->low();

		log("reservoir line OPENED / boiler line CLOSED");

		_resLineClosed = false;
	}
	
	void waitMotorTempStop()
	{
		while (_tempMotorSense->isLow())
			usleep(1000);
	}

	void tempValveHot()
	{
		log("mix line OPENING for more HEAT");

		_motorTempDown->low();
		waitMotorTempStop();

		_motorTempUp->high();
	}

	void tempValveCold()
	{
		log("mix line OPENING for more COLD");

		_motorTempUp->low();
		waitMotorTempStop();
	
		_motorTempDown->high();
	}

	void tempValveOff()
	{
		_motorTempDown->low();
		_motorTempUp->low();

		log("mix line fixed");
	}

	bool isTempMotorOn()
	{
		return _tempMotorSense->isLow();
	}
};

