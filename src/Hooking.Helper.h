#pragma once

namespace hook
{
	/// `addr` should point to the relative address, not to the first byte of the instruction.
	template<typename T = void>
	inline T* get_absolute_address(void* addr)
	{
		char* from = reinterpret_cast<char*>(addr);
		int relAddr = *reinterpret_cast<int*>(addr);
		return reinterpret_cast<T*>(from + relAddr + 4);
	}
}