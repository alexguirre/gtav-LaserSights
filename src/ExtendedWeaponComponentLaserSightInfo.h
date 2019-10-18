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
	rage::Vec3V Color;
	float MinVisibility;
	float MaxVisibility;
	uint32_t CoronaColor;
	bool DebugLines;

public:
	static constexpr size_t NumMembers{ 9 };

	static rage::parMemberCommonData* MemberData[NumMembers + 1]; // null terminated array
	static uint32_t MemberOffsets[NumMembers];

	static void InstallHooks();
};
static_assert(sizeof(ExtendedWeaponComponentLaserSightInfo) <= 192); // 192 = size of each slot in the CWeaponComponentInfo pool