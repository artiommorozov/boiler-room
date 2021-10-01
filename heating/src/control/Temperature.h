#ifdef max
#undef max
#endif

namespace Heat
{

	class Temperature : boost::noncopyable
	{
		Config _cfg;

		enum SensorId
		{
			ROOM_TEMP = 0,

			FURNACE_TEMP_A,
			FURNACE_TEMP_B,
			FURNACE_RETURN,

			MIX_OUT,
			MIX_RESERVOIR,
			MIX_RETURN,

			RESERVOIR_LOW,

			RADIATORS_RET_A,
			RADIATORS_RET_B
		};

		std::vector< std::shared_ptr< Temp::ISensor > > _sensors;
		std::vector< int > _readings;

		void _pick(const std::vector< std::shared_ptr< Temp::ISensor > > &from,
			const std::string &id, int index)
		{
			for (const auto &i : from)
				if (i->id().toString() == id)
				{
					_sensors.resize(std::max(_sensors.size(), (size_t) (index + 1)));
					_sensors[index] = i;
					return;
				}
		}

		void _loadSensors(const std::vector< std::shared_ptr< Temp::ISensor > > &unordered)
		{
			_pick(unordered, _cfg.roomTemp, ROOM_TEMP);
			_pick(unordered, _cfg.furnaceTempA, FURNACE_TEMP_A);
			_pick(unordered, _cfg.furnaceTempB, FURNACE_TEMP_B);
			_pick(unordered, _cfg.furnaceReturnTemp, FURNACE_RETURN);
			_pick(unordered, _cfg.mixOutTemp, MIX_OUT);
			_pick(unordered, _cfg.mixReservoirTemp, MIX_RESERVOIR);
			_pick(unordered, _cfg.mixReturnTemp, MIX_RETURN);
			_pick(unordered, _cfg.reservoirLowTemp, RESERVOIR_LOW);
			_pick(unordered, _cfg.radiatorReturnTempA, RADIATORS_RET_A);
			_pick(unordered, _cfg.radiatorReturnTempB, RADIATORS_RET_B);
		}

		std::vector<int> _bulkRead(std::string *errors)
		{
			try
			{
				return Temp::bulkRead(_sensors);
			}
			catch (const std::exception &e)
			{
				*errors += std::string(", ") + e.what();
			}

			return std::vector<int>(_readings.size(), 0);
		}

		std::vector<int> _join(const std::vector< std::vector< int > > &src)
		{
			std::vector<int> ret;

			for (size_t i = 0; i < src[0].size(); ++i)
			{
				std::vector< int > values(src.size(), 0);
				for (size_t j = 0; j < values.size(); ++j)
					values[j] = src[j][i];

				std::sort(values.begin(), values.end(), [](int a, int b) { return b < a; });

				size_t goodFrom = 0, goodUntil = 1;

				constexpr int maxTempDelta = 2;
				constexpr size_t minMatchingReadings = 3;

				for (size_t j = 1; j < values.size(); ++j)
					if (values[j - 1] - values[j] >= maxTempDelta)
					{
						if (j - goodFrom >= minMatchingReadings)
						{
							goodUntil = j;
							break;
						}
						else
							goodFrom = j;
					}
					else
						goodUntil = j + 1;

				if (goodUntil <= goodFrom || goodUntil - goodFrom < minMatchingReadings)
					throw std::runtime_error(std::string("Failed reading from sensor ") + _sensors[i]->id().toString() + ", no stable value");

				ret.push_back(std::accumulate(values.begin() + goodFrom, values.begin() + goodUntil, 0)
					/ (goodUntil - goodFrom));
			}

			return ret;
		}

	public:
		explicit Temperature(const Config &cfg)
			: _cfg(cfg)
		{
			auto i = Temp::openAllDs2482_800(_cfg.ds2482Port, _cfg.ds2482Address);
			for (const auto &j : i)
			{
				auto id = j->id();
				std::cout << "Found temp sensor " << id.toString() << "; reads=" << j->read() << "\n";
			}

			_loadSensors(i);
			read();
		}

		Temperature &read()
		{
			std::string errors;
			std::vector<int> a = _bulkRead(&errors), b = _bulkRead(&errors),
				c = _bulkRead(&errors), d = _bulkRead(&errors);

			try
			{
				_readings = _join({ a, b, c, d });
				logTemp(dumpReadings());
			}
			catch (const std::exception &e)
			{
				throw std::runtime_error(std::string("Failed to read temperature, ") + e.what() + errors);
			}

			return *this;
		}

		std::string dumpReadings()
		{
			std::stringstream out;

			out << "";
			for (size_t i = 0; i < _readings.size(); ++i)
				out << _sensors[i]->id().toString() << "," << _readings[i] << ",";

			return out.str();
		}

		bool furnaceHot() const
		{
			return _readings[FURNACE_TEMP_A] > _cfg.furnaceMaxTemp
				|| _readings[FURNACE_TEMP_B] > _cfg.furnaceMaxTemp;
		}

		bool heatFlowsFromFurnaceToReservoir() const
		{
			return _readings[RESERVOIR_LOW] < _readings[FURNACE_TEMP_A] && reservoirHot();
		}

		bool reservoirHot() const
		{
			return _readings[RESERVOIR_LOW] >= _cfg.reservoirLowMaxTemp;
		}

		bool circulationGood() const
		{
			return abs(_readings[FURNACE_TEMP_A] - _readings[FURNACE_TEMP_B]) <= _cfg.furnaceMaxOutDiff
				&& abs(_readings[FURNACE_TEMP_A] - _readings[FURNACE_RETURN]) <= _cfg.furnaceMaxOutReturnDiff;
		}

		int mixCold() const
		{
			return _readings[MIX_RETURN];
		}

		int mixHot() const
		{
			return _readings[MIX_RESERVOIR];
		}
	
		int mixActual() const
		{
			return _readings[MIX_OUT];
		}

		int inside() const
		{
			return _readings[ROOM_TEMP];
		}
	};

}