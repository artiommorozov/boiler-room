#ifndef wait_cycle_h_included
#define wait_cycle_h_included

#include <unistd.h>

template< int TotalWaitMsec = 5 * 1000, int IntervalMsec = 50 >
struct PeriodicCheckWait 
{
	template< typename T >
	bool waitUntil(const T &good)
	{
		const int maxRetries = TotalWaitMsec / IntervalMsec;

		for (int i = 0; i < maxRetries; ++i)
		{
			if (!good())
				usleep(IntervalMsec * 1000);
			else
				return true;
		}

		return false;
	}
};

#endif