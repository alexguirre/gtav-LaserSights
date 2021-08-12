#include "CReplay.h"
#include "Addresses.h"

bool CReplay::IsRecordingActive()
{
	return reinterpret_cast<bool(*)()>(Addresses.CReplay_IsRecordingActive)();
}

CPacketWeaponFlashLight::CPacketWeaponFlashLight()
{
	// CPacket
	size = sizeof(CPacketWeaponFlashLight);
	sizeHi = 0;
	typeId = 102; // 102 = CPacketWeaponFlashLight
	field_8 = 0xAAAAAAAA;
	field_7 = 0;
	field_2 = 0;
	flags = 0;
	gameTime = 0;
	field_14 = 0;
	field_C = 0xACACACAC;
	// CPacketWeaponFlashLight
	weaponObjectReplayId = 0xFFFFFFFF;
	isOn = 0;
	unkFlags = 0;
}

void CPacketWeaponFlashLight::AddToRecording(CEntity* const* entities, bool dontAddToMonitorBuffers, bool forceAddPacket)
{
	reinterpret_cast<void(*)(CPacketWeaponFlashLight*, CEntity* const*, bool, bool)>(Addresses.CPacketWeaponFlashLight_AddToRecording)(this, entities, dontAddToMonitorBuffers, forceAddPacket);
}
