#pragma once

namespace rage
{
	class fiDevice
	{
	public:
		static void MakeMemoryFileName(char* dest, int destSize, const void* buffer, int bufferSize, bool freeBufferOnClose, const char* name);
	};
}
