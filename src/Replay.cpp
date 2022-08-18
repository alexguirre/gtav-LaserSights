#include "Replay.h"
#include "Addresses.h"
#include <jitasm.h>
#include <cstdint>
#include "CReplay.h"
#include "CWeaponComponentLaserSight.h"
#include <spdlog/spdlog.h>

class CPacketWeaponLaserSight : public CPacketWeaponFlashLight
{
public:
	rage::Vec4V dirAndLength;

	CPacketWeaponLaserSight();
};

static void AllowWeaponComponentLaserSightRecording()
{
	// game only records CWeaponComponent creation if it is a flashlight or a scope,
	// patch it to also do it if it is a lasersight
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

	auto addr = Addresses.CPacketObjectCreateBase_ctor_weaponComponentClassIdCheckHookLocation;
	memset(addr, 0x90, StubMaxSize); // nop
	memcpy(addr, stubCode, stubCodeSize);
}

// a bit unused in CPacketWeaponFlashLight::unkFlags to indicate if the packet is for a laser sight
static constexpr uint8_t IsLaserSightStatePacketFlag = 0x80;

static void HookPacketWeaponFlashLightReplayHandler()
{
	// re-use the CPacketWeaponFlashLight as the laser sight packet
	// in its replay handler check if IsLaserSightStatePacketFlag is set to decide whether to run the original handler or our custom handler.

	auto addr = (uint8_t*)Addresses.CPacketWeaponFlashLight_ReplayHandler_hookLocation;
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

		static void LaserSightHandler(CPacketWeaponLaserSight* packet, CWeaponComponentLaserSight* laserSight)
		{
			if (laserSight)
			{
				laserSight->State().IsInReplay = true;
				laserSight->State().IsOff = (packet->isOn & 1) == 0;
				laserSight->SetReplayDirAndLength(packet->dirAndLength);
			}
		}
	} stub;

	constexpr size_t StubMaxSize = 0x40;

	auto stubCode = stub.GetCode();
	auto stubCodeSize = stub.GetCodeSize();
	assert(stubCodeSize <= StubMaxSize && "HookPacketWeaponFlashLightReplayHandler stub too big");

	memcpy(addr, stubCode, stubCodeSize);
}

static void PatchWeaponFlashLightAddToRecording()
{
	// patch CPacketWeaponFlashLight::AddToRecording to copy the whole CPacketWeaponLaserSight struct, not just the CPacketWeaponFlashLight part

	static_assert(sizeof(CPacketWeaponLaserSight) == 0x30, "PatchWeaponFlashLightAddToRecording expects sizeof(CPacketWeaponLaserSight) to be 0x30");


	int8_t* addr = (int8_t*)Addresses.CPacketWeaponFlashLight_AddToRecording;

	assert(*(addr + 0x1A) == 0x40 && "Stack space expected to be 0x40, function changed?");
	constexpr int8_t NewStackSpace = sizeof(CPacketWeaponLaserSight) + 0x20;
	*(addr + 0x1A) = NewStackSpace; // increase stack space from 0x40 (sizeof(CPacketFlashLight)+0x20)

	constexpr int8_t PacketCopyVarOffset = -(int8_t)sizeof(CPacketWeaponLaserSight);
	struct : jitasm::Frontend
	{
		// the jmp(Reg64) from jitasm:: crashes when used, replacing I_JMP to I_CALL fixes it (and still produces a jmp instruction)
		void jmp(const Reg64& dst) { AppendInstr(jitasm::I_CALL, 0xFF, 0, jitasm::Imm8(4), R(dst)); }

		void InternalMain() override
		{
			lea(rcx, qword_ptr[rbp+PacketCopyVarOffset]); // dest
			mov(rdx, rdi); // src
			mov(rax, reinterpret_cast<uintptr_t>(&CopyPacket));
			call(rax);
			
			xor(si, si);
			xor(di, di);
			mov(rax, reinterpret_cast<uintptr_t>(Addresses.CPacketWeaponFlashLight_AddToRecording)+0xD8);
			jmp(rax);
		}

		static void CopyPacket(CPacketWeaponFlashLight* dest, CPacketWeaponFlashLight* src)
		{
			if (src->unkFlags & IsLaserSightStatePacketFlag)
			{
				*reinterpret_cast<CPacketWeaponLaserSight*>(dest) = *reinterpret_cast<CPacketWeaponLaserSight*>(src);
			}
			else
			{
				*dest = *src;
			}
		}
	} stub;

	constexpr size_t StubMaxSize = 0x35;

	auto stubCode = stub.GetCode();
	auto stubCodeSize = stub.GetCodeSize();
	assert(stubCodeSize <= StubMaxSize && "HookPacketWeaponFlashLightReplayHandler stub too big");
	
	memcpy(addr + 0xA3, stubCode, stubCodeSize);
	
	// apply the new stack offset to the remaining code
	*(addr + 0xDF) = PacketCopyVarOffset;
	*(addr + 0xFE) = PacketCopyVarOffset + offsetof(CPacketWeaponLaserSight, field_8);
	*(addr + 0x107) = PacketCopyVarOffset + offsetof(CPacketWeaponLaserSight, field_C);
	*(addr + 0x110) = PacketCopyVarOffset + offsetof(CPacketWeaponLaserSight, weaponObjectReplayId);
	*(addr + 0x124) = PacketCopyVarOffset + offsetof(CPacketWeaponLaserSight, typeId);
	*(addr + 0x15B) = PacketCopyVarOffset;

	// epilogue
	*(addr + 0x17A) = NewStackSpace + 0x20;// 0x70;
	*(addr + 0x17F) = NewStackSpace + 0x28;// 0x78;
	*(addr + 0x184) = NewStackSpace + 0x30;// 0x80; FIXME: this ends up like `mov rdi, qword ptr [rsp - 0x80]` instead of `mov rdi, qword ptr [rsp + 0x80]`
	*(addr + 0x18A) = NewStackSpace;
}

bool Replay::InstallHooks()
{
	AllowWeaponComponentLaserSightRecording();
	HookPacketWeaponFlashLightReplayHandler();
	PatchWeaponFlashLightAddToRecording();
	return true;
}

void Replay::RecordLaserSightState(rage::fwEntity* weaponObject, bool isOn, const rage::Vec4V& dirAndLength)
{
	if (CReplay::IsRecordingActive())
	{
		CPacketWeaponLaserSight packet{};
		packet.isOn = isOn ? 1 : 0;
		packet.unkFlags |= IsLaserSightStatePacketFlag;
		packet.dirAndLength = dirAndLength;
		rage::fwEntity* const entities[]{ weaponObject, nullptr };
		if (entities[0])
		{
			packet.AddToRecording(entities, false, false);
		}
	}
}

CPacketWeaponLaserSight::CPacketWeaponLaserSight() : CPacketWeaponFlashLight()
{
	size = sizeof(CWeaponComponentLaserSight);
}