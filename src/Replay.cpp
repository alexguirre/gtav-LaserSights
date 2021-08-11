#include "Replay.h"
#include <Hooking.Patterns.h>
#include <jitasm.h>
#include <cstdint>
#include "CReplay.h"
#include "CWeaponComponentLaserSight.h"
#include <spdlog/spdlog.h>

static void AllowWeaponComponentLaserSightRecording()
{
	// game only records CWeaponComponent creation if it is a flashlight or a scope,
	// patch it to also do it if it is a lasersight
	auto addr = hook::get_pattern("83 F8 02 74 16 48 8B 86 ? ? ? ? 48 8B 48 08");

	struct : jitasm::Frontend
	{
		void InternalMain() override
		{
			// eax = weaponComponent->GetClassId();

			cmp(eax, 2); // CWeaponComponentFlashLight
			je("origJNE");
			cmp(eax, 3); // CWeaponComponentLaserSight
			je("origJNE");
			cmp(eax, 5); // CWeaponComponentScope

			L("origJNE");
		}
	} stub;

	constexpr size_t StubMaxSize = 0x19;

	auto stubCode = stub.GetCode();
	auto stubCodeSize = stub.GetCodeSize();
	assert(stubCodeSize <= StubMaxSize);

	memset(addr, 0x90, StubMaxSize); // nop
	memcpy(addr, stubCode, stubCodeSize);
}

// a bit unused in CPacketWeaponFlashLight::unkFlags to indicate if the packet is for a laser sight
static constexpr uint8_t IsLaserSightStatePacketFlag = 0x80;

static void HookPacketWeaponFlashLightReplayHandler()
{
	// re-use the CPacketWeaponFlashLight as the laser sight packet
	// in its replay handler check if IsLaserSightStatePacketFlag is set to decide whether to run the original handler or our custom handler.

	auto addr = hook::get_pattern<uint8_t>("4C 8B 82 ? ? ? ? 4D 85 C0 74 33 8A 51 1D 41 8A 48 49");
	static const int32_t CWeapon_ComponentFlashLightOffset = *reinterpret_cast<int32_t*>(addr + 3);
	static const int32_t CWeapon_ComponentLaserSightOffset = CWeapon_ComponentFlashLightOffset + 8;

	struct : jitasm::Frontend
	{
		// the jmp(Reg64) from jitasm:: crashes when used, replacing I_JMP to I_CALL fixes it (and still produces a jmp instruction)
		void jmp(const Reg64& dst) { AppendInstr(jitasm::I_CALL, 0xFF, 0, jitasm::Imm8(4), R(dst)); }

		void InternalMain() override
		{
			// rcx = CPacketWeaponFlashLight*
			// rdx = CWeapon* 

			test(byte_ptr[rcx + offsetof(CPacketWeaponFlashLight, unkFlags)], IsLaserSightStatePacketFlag);
			jz("isFlashLight");

			L("isLaserSight");
			{
				mov(rdx, qword_ptr[rdx + CWeapon_ComponentLaserSightOffset]);
				mov(rax, reinterpret_cast<uintptr_t>(&LaserSightHandler));
				jmp(rax);
			}

			L("isFlashLight");
			{
				mov(rdx, qword_ptr[rdx + CWeapon_ComponentFlashLightOffset]);
				mov(rax, reinterpret_cast<uintptr_t>(&FlashLightHandler));
				jmp(rax);
			}
		}

		static void FlashLightHandler(CPacketWeaponFlashLight* packet, uint8_t* flashLight)
		{
			// original code from the CPacketWeaponFlashLight replay handler
			if (flashLight)
			{
				constexpr int32_t State = 0x49;
				constexpr int32_t unkFlags = 0x48;
				*(flashLight + State) = packet->isOn & 1 | *(flashLight + State) & 0xFC | (*(flashLight + State) | (2 * ((*(flashLight + State) & 1) != (packet->isOn & 1)))) & 2;
				*(flashLight + unkFlags) = packet->unkFlags;
			}
		}

		static void LaserSightHandler(CPacketWeaponFlashLight* packet, CWeaponComponentLaserSight* laserSight)
		{
			if (laserSight)
			{
				laserSight->m_IsOff = !(packet->isOn & 1);
			}
		}
	} stub;

	constexpr size_t StubMaxSize = 0x40;

	auto stubCode = stub.GetCode();
	auto stubCodeSize = stub.GetCodeSize();
	assert(stubCodeSize <= StubMaxSize);

	memcpy(addr, stubCode, stubCodeSize);
}

bool Replay::InstallHooks()
{
	AllowWeaponComponentLaserSightRecording();
	HookPacketWeaponFlashLightReplayHandler();
	return true;
}

void Replay::RecordLaserSightState(rage::fwEntity* weaponObject, bool isOn)
{
	if (CReplay::IsRecordingActive())
	{
		CPacketWeaponFlashLight packet{};
		packet.isOn = isOn;
		packet.unkFlags |= IsLaserSightStatePacketFlag;
		CEntity* entities[]{ reinterpret_cast<CEntity*>(weaponObject), nullptr };
		if (entities[0])
		{
			packet.AddToRecording(entities, false, false);
		}
	}
}
