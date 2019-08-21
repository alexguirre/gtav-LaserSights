#pragma once
#include <stdint.h>
#include "Vec3V.h"
#include "fwEntity.h"

class CWeaponBoneId
{
public:
	uint16_t Id;
};
static_assert(sizeof(CWeaponBoneId) == 0x2);

class CWeaponComponentLaserSightInfo
{
public:
	uint8_t padding_8[0x8];
	uint32_t Name;
	uint32_t Model;
	uint32_t LocName;
	uint32_t LocDesc;
	CWeaponBoneId AttachBone;
	uint8_t padding_22[0x6];
	void* AccuracyModifier;
	void* DamageModifier;
	void* FallOffModifier;
	bool bShownOnWheel;
	bool CreateObject;
	bool ApplyWeaponTint;
	int8_t HudDamage;
	int8_t HudSpeed;
	int8_t HudCapacity;
	int8_t HudAccuracy;
	int8_t HudRange;
	// -----
	float CoronaSize;
	float CoronaIntensity;
	CWeaponBoneId LaserSightBone;
	uint8_t padding_52[0x6];

	virtual ~CWeaponComponentLaserSightInfo() = 0;
	virtual bool GetIsClassId(uint32_t classId) = 0;
	virtual uint32_t GetClassId() = 0;
	virtual void* parser_GetStructure() = 0;
};
static_assert(sizeof(CWeaponComponentLaserSightInfo) == 0x58);

class CWeaponComponentLaserSight
{
public:
	CWeaponComponentLaserSightInfo* m_ComponentInfo;
	rage::fwEntity* m_OwnerWeapon;
	rage::fwEntity* m_ComponentObject;
	uint8_t padding_20[0x10];
	uint32_t m_LaserSightBoneIndex;
	uint8_t padding_34[0xC];
	void* m_RaycastResult;
	rage::Vec3V* m_RaycastHitPosition;
	bool m_HasRaycastHit;
	uint8_t padding_51[0x7];

	virtual bool GetIsClassId(uint32_t classId) = 0;
	virtual uint32_t GetClassId() = 0;
	virtual ~CWeaponComponentLaserSight() = 0;
	virtual void Process(void* entity) = 0;
	virtual void ProcessPostPreRender(void* entity) = 0;
	virtual void ApplyAccuracyModifier(float*) = 0;
	virtual void ApplyDamageModifier(float*) = 0;
	virtual void ApplyFallOffModifier(float*, float*) = 0;
};
static_assert(sizeof(CWeaponComponentLaserSight) == 0x58);
