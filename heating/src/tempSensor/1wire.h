#pragma once

#include <stdexcept>
#include <string>
#include <unistd.h>
#include <memory.h>
#include "util/Noncopyable.h"
#include "util/Hexstr.h"

namespace OneWire
{
	struct Rom
	{
		unsigned char value[0x8];

		std::string toString() const
		{
			return hexStr(value, sizeof(value));
		}

		bool operator <(const Rom &other) const
		{
			return memcmp(value, other.value, sizeof(value)) < 0;
		}
	};

	namespace Commands
	{
		const unsigned char 
			searchRom = 0xf0,
			matchRom = 0x55,
			skipRom = 0xcc;
	};

	template< typename OneWirePort >
	class Session : Noncopyable
	{
		OneWirePort *_port;
		std::string _name;

		std::exception exception(const std::exception &e) const
		{
			throw std::runtime_error(_name + " on channel " + std::to_string(_port->index()) + ", err=" + e.what());
		}

	public:
		explicit Session(OneWirePort &port, const std::string &name, bool reset = true)
			: _port(&port), _name(name)
		{
			try
			{
				_port->connect();

				if (reset)
					_port->reset();
			}
			catch (const std::exception &e)
			{
				throw exception(e);
			}
		}

		Session(Session<OneWirePort> &&other)
		{
			_port = other._port;
			_name = other._name;

			other._port = nullptr;
		}

		~Session()
		{
		}

		template< typename T >
		void perform(T f)
		{
			try
			{
				f(*_port);
			}
			catch (const std::exception &e)
			{
				throw exception(e);
			}
		}
	};

	template< typename Port >
	Session<Port> session(Port &p, const std::string &description, bool reset = true)
	{
		return Session<Port>(p, description, reset);
	}
}

