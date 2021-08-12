#include "CCoronas.h"
#include "Addresses.h"

void CCoronas::Draw(const rage::Vec3V& position, float size, uint32_t color, float intensity, float zbias, const rage::Vec3V& direction, float a8, float innerAngle, float outerAngle, uint16_t flags)
{
	using Fn = void(*)(CCoronas*, const rage::Vec3V&, float, uint32_t, float, float, const rage::Vec3V&, float, float, float, uint16_t);
	reinterpret_cast<Fn>(Addresses.CCoronas_Draw)(this, position, size, color, intensity, zbias, direction, a8, innerAngle, outerAngle, flags);
}

CCoronas* CCoronas::Instance()
{
	return reinterpret_cast<CCoronas*>(Addresses.CCoronas_Instance);
}