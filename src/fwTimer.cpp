#include "fwTimer.h"
#include "Addresses.h"

namespace rage
{
	fwTimeSet& fwTimer::GameTime()
	{
		return *reinterpret_cast<fwTimeSet*>(Addresses.fwTimer_sm_gameTime);
	}
}