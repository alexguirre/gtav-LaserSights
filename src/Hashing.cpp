#include "Hashing.h"
#include "Addresses.h"

namespace rage
{
	uint32_t atDataHash(const void* data, uint64_t dataByteCount, uint32_t initialHash)
	{
		using Fn = decltype(&atDataHash);
		return reinterpret_cast<Fn>(Addresses::atDataHash)(data, dataByteCount, initialHash);
	}
}