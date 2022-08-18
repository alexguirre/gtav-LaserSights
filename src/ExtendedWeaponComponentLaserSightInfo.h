#pragma once
#include "CWeaponComponentLaserSight.h"
#include <cstddef>

namespace rage
{
	class parMemberCommonData;
}

class ExtendedWeaponComponentLaserSightInfo : public CWeaponComponentLaserSightInfo
{
public:
	float BeamWidth;
	float BeamRange;
	rage::Vec3V Color;
	uint32_t CoronaColor;
	bool IR;

public:
	static constexpr size_t NumMembers{ 8 };

	static rage::parMemberCommonData* MemberData[NumMembers + 1]; // null terminated array
	static uint32_t MemberOffsets[NumMembers];

	static bool InstallHooks();
};
static_assert(sizeof(ExtendedWeaponComponentLaserSightInfo) <= 192); // 192 = size of each slot in the CWeaponComponentInfo pool