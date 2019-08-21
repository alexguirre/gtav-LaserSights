#include "CCoronas.h"
#include <Hooking.Patterns.h>


void CCoronas::Draw(const rage::Vec3V& position, float size, uint32_t color, float intensity, float zbias, const rage::Vec3V& direction, float a8, float innerAngle, float outerAngle, uint16_t flags)
{
	static void* addr = hook::get_pattern("44 89 4C 24 ? 48 83 EC 28 0F 29 74 24 ?");

	((void(*)(CCoronas*, const rage::Vec3V&, float, uint32_t, float, float, const rage::Vec3V&, float, float, float, uint16_t))addr)(this, position, size, color, intensity, zbias, direction, a8, innerAngle, outerAngle, flags);
}

CCoronas* CCoronas::Instance()
{
	static CCoronas* inst = []()
	{
		auto a = hook::get_pattern<char>("F3 41 0F 59 DD 48 8D 0D ? ? ? ? F3 0F 11 44 24 ?", 8);
		return (CCoronas*)(a + *(int*)a + 4);
	}();

	return inst;
}