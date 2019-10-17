#include "ExtendedWeaponComponentLaserSightInfo.h"
#include "parStructure.h"
#include "Hashing.h"
#include "Addresses.h"
#include <spdlog/spdlog.h>
#include <MinHook.h>

#define OFFSET(fieldName) offsetof(ExtendedWeaponComponentLaserSightInfo, fieldName)

#define DATA_NAME(fieldName) fieldName ## _Data

#define DATA_PTR(fieldName) (&DATA_NAME(fieldName))

#define MEMBER_DATA_SIMPLE(fieldName, type, subType)                      \
	static rage::parMemberSimpleData DATA_NAME(fieldName)                 \
	{                                                                     \
		rage::atLiteralStringHash(#fieldName), {}, OFFSET(fieldName), {}, \
		type, subType, 1, 0xFFFF, 0, nullptr, 0, {}                       \
	}

#define MEMBER_DATA_FLOAT(fieldName) MEMBER_DATA_SIMPLE(fieldName, rage::parMemberType::FLOAT, 0)
#define MEMBER_DATA_UINT(fieldName) MEMBER_DATA_SIMPLE(fieldName, rage::parMemberType::UINT, 1)

#define MEMBER_DATA_STRUCT(fieldName, subType)                            \
	static rage::parMemberStructData DATA_NAME(fieldName)                 \
	{                                                                     \
		rage::atLiteralStringHash(#fieldName), {}, OFFSET(fieldName), {}, \
		rage::parMemberType::STRUCT, subType, 9, 0xFFFF, 0, nullptr,      \
		nullptr, 0, 0, 0                                                  \
	}


MEMBER_DATA_FLOAT(CoronaSize);
MEMBER_DATA_FLOAT(CoronaIntensity);
MEMBER_DATA_STRUCT(LaserSightBone, 0);

// new properties
MEMBER_DATA_FLOAT(BeamWidth);
MEMBER_DATA_UINT(Color);
MEMBER_DATA_FLOAT(MinVisibility);
MEMBER_DATA_FLOAT(MaxVisibility);

rage::parMemberCommonData* ExtendedWeaponComponentLaserSightInfo::MemberData[NumMembers + 1]
{
	DATA_PTR(CoronaSize),
	DATA_PTR(CoronaIntensity),
	DATA_PTR(LaserSightBone),

	// new properties
	DATA_PTR(BeamWidth),
	DATA_PTR(Color),
	DATA_PTR(MinVisibility),
	DATA_PTR(MaxVisibility),

	nullptr // null terminator
};

uint32_t ExtendedWeaponComponentLaserSightInfo::MemberOffsets[NumMembers]
{
	OFFSET(CoronaSize),
	OFFSET(CoronaIntensity),
	OFFSET(LaserSightBone),

	// new properties
	OFFSET(BeamWidth),
	OFFSET(Color),
	OFFSET(MinVisibility),
	OFFSET(MaxVisibility),
};

static rage::parMemberCommonData** origMemberData;
static uint32_t* origMemberOffsets;

static void(*CWeaponComponentLaserSightInfo_parser_Register_orig)();
static void CWeaponComponentLaserSightInfo_parser_Register_detour()
{
	CWeaponComponentLaserSightInfo_parser_Register_orig();

	// get rage::parStructure* for CWeaponBoneId
	rage::parMemberStructData* origLaserSightBoneData = reinterpret_cast<rage::parMemberStructData*>(origMemberData[2]);
	rage::parMemberStructData* newLaserSightBoneData = reinterpret_cast<rage::parMemberStructData*>(ExtendedWeaponComponentLaserSightInfo::MemberData[2]);
	
	newLaserSightBoneData->m_Structure = origLaserSightBoneData->m_Structure;
}

void ExtendedWeaponComponentLaserSightInfo::InstallHooks()
{
	rage::parStructureStaticData* laserSightInfoData = reinterpret_cast<rage::parStructureStaticData*>(Addresses::CWeaponComponentLaserSightInfo_parser_Data);

	assert(laserSightInfoData->m_Structure == nullptr); // ensure the parStructure is not initialized yet

	origMemberData = laserSightInfoData->m_MemberData;
	origMemberOffsets = laserSightInfoData->m_MemberOffsets;

	laserSightInfoData->m_MemberData = MemberData;
	laserSightInfoData->m_MemberOffsets = MemberOffsets;

	*reinterpret_cast<uint32_t*>(Addresses::CWeaponComponentLaserSightInfo_parser_Register_SizeOfConstant) = sizeof(ExtendedWeaponComponentLaserSightInfo);

	MH_CreateHook(Addresses::CWeaponComponentLaserSightInfo_parser_Register, CWeaponComponentLaserSightInfo_parser_Register_detour, reinterpret_cast<void**>(&CWeaponComponentLaserSightInfo_parser_Register_orig));
}
