#pragma once
#include <cstdint>

class CReplay
{
public:
	static bool IsRecordingActive();
};

class CPacket
{
public:
	uint16_t typeId;
	uint8_t field_2;
	uint8_t flags;
	uint16_t size;
	uint8_t sizeHi;
	uint8_t field_7;
	uint32_t field_8;
	uint32_t field_C;
	uint32_t gameTime;
	uint32_t field_14;
};
static_assert(sizeof(CPacket) == 0x18);

class CPacketWeaponFlashLight : public CPacket
{
public:
	uint32_t weaponObjectReplayId;
	uint8_t unkFlags;
	uint8_t isOn;
	uint8_t padding_1E;
	uint8_t padding_1F;

	CPacketWeaponFlashLight();

	void AddToRecording(class CEntity* const* entities, bool dontAddToMonitorBuffers, bool forceAddPacket);
};
static_assert(sizeof(CPacketWeaponFlashLight) == 0x20);
