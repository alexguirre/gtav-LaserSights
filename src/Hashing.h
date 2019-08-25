#pragma once
#include <stdint.h>

namespace rage
{
	uint32_t atDataHash(const void* data, uint64_t dataByteCount, uint32_t initialHash = 0);
}
