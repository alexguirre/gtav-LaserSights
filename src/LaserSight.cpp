#include "LaserSight.h"
#include "CWeapon.h"
#include "CWeaponComponentLaserSight.h"
#include "CCoronas.h"
#include <spdlog/spdlog.h>
#include "Addresses.h"
#include "LaserBeam.h"
#include "ExtendedWeaponComponentLaserSightInfo.h"
#include "camBaseCamera.h"
#include "WorldProbe.h"
#include "CTaskAimGun.h"
#include "Replay.h"
#include <filesystem>
#include "Resources.h"
#include "fwTimer.h"

enum : uint32_t
{
    NONE = 0u,
    UNKNOWN = 1u,
    MAP_WEAPON = 1u << 1,
    MAP_DYNAMIC = 1u << 2,
    MAP_ANIMAL = 1u << 3,
    MAP_COVER = 1u << 4,
    MAP_VEHICLE = 1u << 5,
    VEHICLE_NOT_BVH = 1u << 6,
    VEHICLE_BVH = 1u << 7,
    VEHICLE_BOX = 1u << 8,
    PED = 1u << 9,
    RAGDOLL = 1u << 10,
    ANIMAL = 1u << 11,
    ANIMAL_RAGDOLL = 1u << 12,
    OBJECT = 1u << 13,
    OBJECT_ENV_CLOTH = 1u << 14,
    PLANT = 1u << 15,
    PROJECTILE = 1u << 16,
    EXPLOSION = 1u << 17,
    PICKUP = 1u << 18,
    FOLIAGE = 1u << 19,
    FORKLIFT_FORKS = 1u << 20,
    TEST_WEAPON = 1u << 21,
    TEST_CAMERA = 1u << 22,
    TEST_AI = 1u << 23,
    TEST_SCRIPT = 1u << 24,
    TEST_VEHICLE_WHEEL = 1u << 25,
    GLASS = 1u << 26,
    MAP_RIVER = 1u << 27,
    SMOKE = 1u << 28,
    UNSMASHED = 1u << 29,
    MAP_STAIRS = 1u << 30,
    MAP_DEEP_SURFACE = 1u << 31,
};

static struct Config
{
	uint32_t ShapeTestFlags1 = 0x100;
	uint32_t ShapeTestFlags2 = 
		0xE1134C2; // MAP_WEAPON, VEHICLE_NOT_BVH, VEHICLE_BVH, RAGDOLL, ANIMAL_RAGDOLL, OBJECT, PROJECTILE, FORKLIFT_FORKS, TEST_VEHICLE_WHEEL, GLASS, MAP_RIVER
} g_Config;

// NOTE: CScriptIM::DrawLine crashes if called during R* Editor export
static void CScriptIM_DrawLine(const rage::Vec3V& start, const rage::Vec3V& end, uint32_t color)
{
	reinterpret_cast<decltype(&CScriptIM_DrawLine)>(Addresses.CScriptIM_DrawLine)(start, end, color);
}

static void CleanMat34V(rage::Mat34V& m)
{
	m.m[0][3] = 0.0f;
	m.m[1][3] = 0.0f;
	m.m[2][3] = 0.0f;
	m.m[3][3] = 1.0f;
}

static rage::aiTaskTree* GetPedTaskTree(rage::fwEntity* ped)
{
	// TODO: get offsets from patterns
	constexpr int IntelligenceOffset = 0x10A0; // b2802
	constexpr int TaskManagerOffset = 0x368; // b2802
	constexpr int TaskTreeOffset = 0x8;

	uint8_t* p = reinterpret_cast<uint8_t*>(ped);
	uint8_t* intelligence = *reinterpret_cast<uint8_t**>(p + IntelligenceOffset);
	uint8_t* taskMgr = reinterpret_cast<uint8_t*>(intelligence + TaskManagerOffset);
	rage::aiTaskTree* taskTree = *reinterpret_cast<rage::aiTaskTree**>(taskMgr + TaskTreeOffset);

	return taskTree;
}

static bool IsContextPressed()
{
	class CControl;
	class ioValue;
	constexpr int INPUT_CONTEXT{ 51 };

	CControl* const c = ((CControl * (*)(bool))Addresses.CControlMgr_GetDefaultControl)(true);
	ioValue* const v = ((ioValue * (*)(CControl*, int))Addresses.CControlMgr_GetIoValue)(c, INPUT_CONTEXT);

	constexpr struct ioValue_ReadOptions { float f1; uint32_t f2; } Options{ 0.0f, 1 };
	constexpr float Threshold{ 0.5f };
	return ((bool(*)(ioValue*, float, const ioValue_ReadOptions&))Addresses.ioValue_IsPressed)(v, Threshold, Options);
}

static bool IsNightVisionEnabled()
{
	return *reinterpret_cast<bool*>(Addresses.IsNightVisionEnabled);
}

static void PlayToggleSound(void* weapon, bool isOn)
{
	reinterpret_cast<void(*)(void*, void*, bool)>(Addresses.PlayWeaponFlashLightToggleSound)(Addresses.audWeaponAudioEntity_Instance, weapon, isOn);
}

static void DRAW_SPOT_LIGHT(const rage::Vec3V& pos, const rage::Vec3V& dir, int r, int g, int b, float distance, float brightness, float innerAngle, float outerAngle, float falloff)
{
	struct scrVector3 { float x, _pad1, y, _pad2, z, _pad3; };
	using Fn = void(*)(const scrVector3& pos, const scrVector3& dir, int r, int g, int b, float distance, float brightness, float innerAngle, float outerAngle, float falloff);
	scrVector3 posS{ pos.x, 0, pos.y, 0, pos.z, 0 };
	scrVector3 dirS{ dir.x, 0, dir.y, 0, dir.z, 0 };
	reinterpret_cast<Fn>(Addresses.DRAW_SPOT_LIGHT)(posS, dirS, r, g, b, distance, brightness, innerAngle, outerAngle, falloff);
}

static bool IsLaserVisible(CWeaponComponentLaserSight* sight)
{
	if (sight->State().IsOff)
	{
		return false;
	}

	const auto* info = reinterpret_cast<ExtendedWeaponComponentLaserSightInfo*>(sight->m_ComponentInfo);

	if (info->IR && !IsNightVisionEnabled())
	{
		// if the laser is infrared it is only visible with night vision
		return false;
	}

	return true;
}

static rage::fwEntity* GetWeaponObject(CWeaponComponentLaserSight* sight)
{
	return *reinterpret_cast<rage::fwEntity**>(reinterpret_cast<uint8_t*>(sight->m_OwnerWeapon) + 0x58);
}

static void(*CWeaponComponentLaserSight_Process_orig)(CWeaponComponentLaserSight* This, rage::fwEntity* ped);
static void CWeaponComponentLaserSight_Process_detour(CWeaponComponentLaserSight* This, rage::fwEntity* ped)
{
	if (IsContextPressed())
	{
		auto& s = This->State();
		s.IsOff ^= 1;
		PlayToggleSound(This->m_OwnerWeapon, !s.IsOff);
	}
}

static void(*CWeaponComponentLaserSight_ProcessPostPreRender_orig)(CWeaponComponentLaserSight* This, rage::fwEntity* ped);
static void CWeaponComponentLaserSight_ProcessPostPreRender_detour(CWeaponComponentLaserSight* This, rage::fwEntity* ped)
{
	using namespace DirectX;

	if (!IsLaserVisible(This))
	{
		if (This->m_OwnerWeapon)
		{
			Replay::RecordLaserSightState(GetWeaponObject(This), false, {});
		}
		return;
	}

	if (This->m_OwnerWeapon && This->m_ComponentObject && This->m_LaserSightBoneIndex != -1)
	{
		const auto* info = reinterpret_cast<ExtendedWeaponComponentLaserSightInfo*>(This->m_ComponentInfo);
		rage::Mat34V boneMtx;
		This->m_ComponentObject->GetGlobalMtx(This->m_LaserSightBoneIndex, &boneMtx);

		CCoronas::Instance()->Draw(boneMtx.Position(), info->CoronaSize, info->CoronaColor, info->CoronaIntensity, 100.0f, boneMtx.Forward(), 1.0f, 30.0f, 35.0f, 3);
		
		rage::Vec3V newTargetDir;
		if (This->State().IsInReplay)
		{
			using namespace DirectX;
			// convert the direction from local space to world space
			
			rage::Vec3V dir = This->GetReplayDir();
			rage::Mat34V boneMtxClean = boneMtx;
			CleanMat34V(boneMtxClean);
			newTargetDir = XMVector3TransformNormal(dir.v, boneMtxClean.x);
		}
		else
		{
			// TODO: replace hardcoded offset
			constexpr int PlayerInfoOffset = 0x10A8; // b2802
			const bool isPlayer = ped && *reinterpret_cast<void**>((reinterpret_cast<uint8_t*>(ped) + PlayerInfoOffset));

			newTargetDir = boneMtx.Forward();

			if (ped)
			{
				constexpr uint32_t CTaskAimGunOnFoot = 4;
				constexpr uint32_t CTaskAimGunVehicleDriveBy = 295;
				constexpr uint32_t CTaskAimGunBlindFire = 304;

				rage::aiTaskTree* taskTree = GetPedTaskTree(ped);
				rage::aiTask* task = nullptr;
				if ((task = taskTree->FindTaskByTypeActive(CTaskAimGunOnFoot)) ||
					(task = taskTree->FindTaskByTypeActive(CTaskAimGunVehicleDriveBy)) ||
					(task = taskTree->FindTaskByTypeActive(CTaskAimGunBlindFire)))
				{
					CTaskAimGun* aimTask = reinterpret_cast<CTaskAimGun*>(task);
					rage::Vec3V aimStartPos, aimEndPos;
					bool s = aimTask->DoShapeTest(ped, &aimStartPos, &aimEndPos, nullptr, nullptr, true);

					uint32_t muzzleBoneIndex = This->m_OwnerWeapon->OwnerObject->GetBoneIndex(17833/*Gun_Muzzle*/);
					rage::Mat34V muzzleMtx;
					This->m_OwnerWeapon->OwnerObject->GetGlobalMtx(muzzleBoneIndex, &muzzleMtx);

					//CScriptIM_DrawLine(muzzleMtx.Position(), aimEndPos, s ? 0xFF00FF00 : 0xFF0000FF);
					//CScriptIM_DrawLine(aimStartPos+rage::Vec3V(1.0f,1.0f,1.0f), aimEndPos+rage::Vec3V(1.0f,1.0f,1.0f), s ? 0xFF00FF00 : 0xFF0000FF);

					//const rage::Vec3V origDir = (origEndPos - origStartPos).Normalized();
					const rage::Vec3V aimDir = (aimEndPos - muzzleMtx.Position()).Normalized();

					newTargetDir = aimDir;
				}
			}
		}

		auto& d = This->Data();
		d.TargetDir = newTargetDir;
		d.CurrDir = d.TargetDir;

		{
			// convert the direction from world space to local space
			rage::Vec3V dir = d.TargetDir;
			rage::Mat34V boneMtxInv = boneMtx;
			CleanMat34V(boneMtxInv);
			boneMtxInv = XMMatrixInverse(nullptr, boneMtxInv.x);
			dir = XMVector3TransformNormal(dir.v, boneMtxInv.x);
			Replay::RecordLaserSightState(GetWeaponObject(This), true, dir);
		}

		rage::Vec3V startPos = boneMtx.Position();
		rage::Vec3V endPos = startPos + d.CurrDir * info->BeamRange;

		{
			static WorldProbe::CShapeTestResults results(16);

			results.AbortTest();
			WorldProbe::CShapeTestProbeDesc desc;
			const rage::fwEntity* excludeEntities[]{ This->m_ComponentObject, GetWeaponObject(This) };
			desc.SetExcludeEntities(excludeEntities, std::size(excludeEntities), 0);
			desc.SetResultsStructure(&results);
			// TODO: find more appropriate shapetest flags
			desc.m_Flags1 = g_Config.ShapeTestFlags1;
			desc.m_TypeFlags = g_Config.ShapeTestFlags2;
			desc.m_IncludeFlags = 0xFFFFFFFF;
			desc.m_Start = startPos;
			desc.m_End = endPos;
			desc.m_84C = 8;

			WorldProbe::GetShapeTestManager()->SubmitTest(desc, false);

			if (results.m_State == 4 && results.m_HitCount > 0)
			{
				rage::Vec3V prevHitPos(99999.9f);
				for (int i = 0; i < results.m_HitCount; i++)
				{
					WorldProbe::CShapeTestHit* hit = &results.m_Hits[i];

					endPos = hit->m_Position;
					
					if ((endPos - prevHitPos).LengthSquared() > 0.15f * 0.15f)
					{
						CCoronas::Instance()->Draw(hit->m_Position, info->CoronaSize * 0.2f, info->CoronaColor, info->CoronaIntensity * 1.25f, 100.0f, hit->m_SurfaceNormal, 1.0f, 60.0f, 85.0f, 3);
						//DRAW_SPOT_LIGHT(hit->m_Position + hit->m_SurfaceNormal * 0.125f, -hit->m_SurfaceNormal, 255, 0, 0, 0.15f, 1000.0f, 7.5f, 7.5f, 1.0f);
					}
					prevHitPos = endPos;
				}

				results.AbortTest();
			}
		}

		{
			const rage::Mat34V& camMtx = camBaseCamera::GetCurrentCamera()->GetTransform();

			// based on arbitrary axis billboards: http://nehe.gamedev.net/article/billboarding_how_to/18011/
			const rage::Vec3V center = (startPos + endPos) * 0.5f;
			const rage::Vec3V look = camMtx.Position() - center;
			const rage::Vec3V up = (endPos - startPos).Normalized();
			const rage::Vec3V right = up.Cross(look).Normalized();

			LaserBeam::DrawBeam(info->BeamWidth, startPos, endPos, right, info->Color);
		}
	}
}

static void(*CWeaponComponentLaserSight_ApplyAccuracyModifier_orig)(CWeaponComponentLaserSight* This, void* accuracyStruct);
static void CWeaponComponentLaserSight_ApplyAccuracyModifier_detour(CWeaponComponentLaserSight* This, void* accuracyStruct)
{
	if (!IsLaserVisible(This))
	{
		return;
	}

	// only apply the accuracy modifier is the laser is visible
	CWeaponComponentLaserSight_ApplyAccuracyModifier_orig(This, accuracyStruct);
}

bool LaserSight::InstallHooks()
{
	void** vtable = (void**)Addresses.CWeaponComponentLaserSight_vftable;

	constexpr size_t Idx_Process = 8;
	constexpr size_t Idx_ProcessPostPreRender = 9;
	constexpr size_t Idx_ApplyAccuracyModifier = 10;

	CWeaponComponentLaserSight_Process_orig = (decltype(CWeaponComponentLaserSight_Process_orig))vtable[Idx_Process];
	CWeaponComponentLaserSight_ProcessPostPreRender_orig = (decltype(CWeaponComponentLaserSight_ProcessPostPreRender_orig))vtable[Idx_ProcessPostPreRender];
	CWeaponComponentLaserSight_ApplyAccuracyModifier_orig = (decltype(CWeaponComponentLaserSight_ApplyAccuracyModifier_orig))vtable[Idx_ApplyAccuracyModifier];

	vtable[Idx_Process] = CWeaponComponentLaserSight_Process_detour;
	vtable[Idx_ProcessPostPreRender] = CWeaponComponentLaserSight_ProcessPostPreRender_detour;
	vtable[Idx_ApplyAccuracyModifier] = CWeaponComponentLaserSight_ApplyAccuracyModifier_detour;

	return true;
}
