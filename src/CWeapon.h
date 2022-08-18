#pragma once
#include "Vector.h"

class CWeapon
{
public:
	uint64_t m_8;
	rage::Vec4V Rotation; // actually rage::QuatV
	rage::Vec3V Position;
	uint64_t m_30;
	uint64_t m_38;
	void* WeaponInfo;
	// ...

	virtual ~CWeapon() = 0;
	virtual bool Fire(const void* params) = 0;
};
