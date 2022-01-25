#include "fiDevice.h"
#include <cstdio>

namespace rage
{
	void fiDevice::MakeMemoryFileName(char* dest, int destSize, const void* buffer, int bufferSize, bool freeBufferOnClose, const char* name)
	{
		int nFreeBufferOnClose = freeBufferOnClose ? 1 : 0;
		std::snprintf(dest, destSize, "memory:$%p,%d,%d:%s", buffer, bufferSize, nFreeBufferOnClose, name);
	}
}
