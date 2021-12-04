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
		constexpr int
			polls = 10,
			timeoutMsec = 1;

		int ones = 0;
		for (int i = 0; i < polls; ++i)
		{
			_f.seekg(0);

			std::string v;
			std::getline(_f, v);
			if (v == "1")
				++ones;
			else if (v != "0")
				throw std::runtime_error(std::string("Reading from pin ") + name() + " produced invalid value=" + v);

			usleep(timeoutMsec * 1000);
		}

		return ones < polls / 2;
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

		_f.seekp(0);
		_f << "0\n";
		_f.flush();
		_low = true;

		return ret;
	}

	bool high()
	{
		bool ret = _low;

		_f.seekp(0);
		_f << "1\n";
		_f.flush();
		_low = false;

		return ret;
	}

	bool isLow() const
	{
		return _low;
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
		_diesel, _dieselValve, _furnacePump, _pumpValve, _boilerValve, _circulationPump, _radiatorPump, _elHeater;

	std::unique_ptr< GpioInPin > _tempMotorSense, _boilerSense;

	bool _resLineClosed;

public:
	explicit Gpio(const Config &cfg)
		: _resLineClosed(false)
	{
		for (int x : cfg.gpioExports)
			_export(x);

		_motorTempUp.reset(new GpioOutPin(cfg.gpioMotorTempUp));
		_motorTempDown.reset(new GpioOutPin(cfg.gpioMotorTempDown));
		_motorCloseBoiler.reset(new GpioOutPin(cfg.gpioMotorCloseBoiler));
		_motorOpenBoiler.reset(new GpioOutPin(cfg.gpioMotorOpenBoiler));
		_diesel.reset(new GpioOutPin(cfg.gpioDiesel));
		_dieselValve.reset(new GpioOutPin(cfg.gpioDieselValve));
		_furnacePump.reset(new GpioOutPin(cfg.gpioHeatersPump));
		_pumpValve.reset(new GpioOutPin(cfg.gpioPumpValve));
		_boilerValve.reset(new GpioOutPin(cfg.gpioBoilerValve));
		_circulationPump.reset(new GpioOutPin(cfg.gpioCirculationPump));
		_radiatorPump.reset(new GpioOutPin(cfg.gpioRadiatorPump));
		_elHeater.reset(new GpioOutPin(cfg.gpioElectricHeater));

		_tempMotorSense.reset(new GpioInPin(cfg.gpioTempMotorSense));
		_boilerSense.reset(new GpioInPin(cfg.gpioBoilerSense));

		log("GPIO init complete");
	}

	void dieselOff()
	{
		if (_diesel->low())
			log("diesel off");
	}

	void dieselOn()
	{
		dieselValveOpen();

		_diesel->high();

		log("diesel ON");
	}

	void electricHeaterOff()
	{
		if (_elHeater->low())
			log("electric heater off");
	}

	void electricHeaterOn()
	{
		_elHeater->high();

		log("electric heater ON");
	}

	void dieselValveClose()
	{
		if (_dieselValve->low())
			log("diesel valve closed");
	}

	void dieselValveOpen()
	{
		_dieselValve->high();
		log("diesel valve OPEN");
	}

	bool isDieselOn() const
	{
		return !_diesel->isLow();
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

	bool isFurnacePumpOn() const
	{
		return !_furnacePump->isLow();
	}

	void boilerValveOpen()
	{
		_boilerValve->high();
		log("boiler valve OPEN");
		sleep(1);
	}

	void boilerValveClose()
	{
		_boilerValve->low();
		log("boiler valve closed");
	}

	void pumpValveOpen()
	{
		_pumpValve->high();
		log("pump valve OPEN");
		sleep(1);
	}

	void pumpValveClose()
	{
		_pumpValve->low();
		log("pump valve closed");
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

	void closeReservoirLineBegin()
	{
		if (_resLineClosed)
			return;

		log("reservoir line closing / boiler line open");

		_motorCloseBoiler->low();
		sleep(1);
		_motorOpenBoiler->high();
	}

	void closeReservoirLineEnd()
	{
		if (_resLineClosed)
			return;

		_motorOpenBoiler->low();

		log("reservoir line CLOSED / boiler line OPENED");

		_resLineClosed = true;
	}

	void openReservoirLineBegin()
	{
		if (!_resLineClosed)
			return;

		log("reservoir line open / boiler line closing");

		_motorOpenBoiler->low();
		sleep(1);
		_motorCloseBoiler->high();
	}

	void openReservoirLineEnd()
	{
		if (!_resLineClosed)
			return;

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

