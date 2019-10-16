#pragma once
#include <stdint.h>

namespace rage
{
	uint32_t atDataHash(const void* data, uint64_t dataByteCount, uint32_t initialHash = 0);
	
	constexpr uint32_t atLiteralStringHash(const char* key, uint32_t initialHash = 0)
	{
		char c = 0;
		while (c = *key)
		{
			initialHash = (1025 * (c + initialHash) >> 6) ^ 1025 * (c + initialHash);
			key++;
		}
		return 32769 * ((9 * initialHash >> 11) ^ 9 * initialHash);
	}
}
