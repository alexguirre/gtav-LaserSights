#pragma once
#include "Vector.h"

namespace rage
{
	class fwEntity;
}

class CWeapon
{
public:
	rage::Vec4V Rotation; // actually rage::QuatV
	rage::Vec3V Position;
	uint64_t m_30;
	uint64_t m_38;
	void* WeaponInfo;
	uint64_t m_48;
	uint64_t m_50;
	rage::fwEntity* OwnerObject; // actually CObject
	// ...

	virtual ~CWeapon() = 0;
	virtual bool Fire(const void* params) = 0;
};
