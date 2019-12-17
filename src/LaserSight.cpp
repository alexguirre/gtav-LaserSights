#include "LaserSight.h"
#include <Hooking.Patterns.h>
#include "CWeaponComponentLaserSight.h"
#include "CCoronas.h"
#include <spdlog/spdlog.h>
#include "Hooking.Helper.h"
#include "Addresses.h"
#include "LaserBeam.h"
#include "ExtendedWeaponComponentLaserSightInfo.h"
#include "camBaseCamera.h"
#include "WorldProbe.h"
#include "CTaskAimGun.h"

static void CScriptIM_DrawLine(const rage::Vec3V& start, const rage::Vec3V& end, uint32_t color)
{
	reinterpret_cast<decltype(&CScriptIM_DrawLine)>(Addresses::CScriptIM_DrawLine)(start, end, color);
}

static rage::aiTaskTree* GetPedTaskTree(rage::fwEntity* ped)
{
	// TODO: get offsets from patterns
	static const int IntelligenceOffset = 0x10B0;
	static const int TaskManagerOffset = 0x370; // v1868
	static const int TaskTreeOffset = 0x0;

	uint8_t* p = reinterpret_cast<uint8_t*>(ped);
	uint8_t* intelligence = *reinterpret_cast<uint8_t**>(p + IntelligenceOffset);
	uint8_t* taskMgr = reinterpret_cast<uint8_t*>(intelligence + TaskManagerOffset);
	rage::aiTaskTree* taskTree = *reinterpret_cast<rage::aiTaskTree**>(taskMgr + TaskTreeOffset);

	return taskTree;
}

static void(*CWeaponComponentLaserSight_Process_orig)(CWeaponComponentLaserSight* This, rage::fwEntity* entity);
static void CWeaponComponentLaserSight_Process_detour(CWeaponComponentLaserSight* This, rage::fwEntity* entity)
{
	CWeaponComponentLaserSight_Process_orig(This, entity);
}

static void(*CWeaponComponentLaserSight_ProcessPostPreRender_orig)(CWeaponComponentLaserSight* This, rage::fwEntity* entity);
static void CWeaponComponentLaserSight_ProcessPostPreRender_detour(CWeaponComponentLaserSight* This, rage::fwEntity* entity)
{
	CWeaponComponentLaserSight_ProcessPostPreRender_orig(This, entity);

	if (This->m_OwnerWeapon && This->m_ComponentObject && This->m_LaserSightBoneIndex != -1)
	{
		const auto* info = reinterpret_cast<ExtendedWeaponComponentLaserSightInfo*>(This->m_ComponentInfo);
		const rage::Vec3V beamColor(info->Color);
		const rage::Vec4V dotColor(
			beamColor.x,
			beamColor.y,
			beamColor.z,
			info->MaxVisibility * 0.55f
		);
		const bool isPlayer = entity && *reinterpret_cast<void**>((reinterpret_cast<uint8_t*>(entity) + 0x10B8)); // TODO: replace hardcoded offset

		rage::Mat34V boneMtx;
		This->m_ComponentObject->GetGlobalMtx(This->m_LaserSightBoneIndex, &boneMtx);

		rage::Vec3V startPos = boneMtx.Position();
		rage::Vec3V endPos = startPos + boneMtx.Forward() * info->BeamRange;

		if (info->DebugLines)
		{
			CScriptIM_DrawLine(startPos, endPos, 0xFFFF0000);
		}

		CCoronas::Instance()->Draw(startPos, info->CoronaSize, info->CoronaColor, info->CoronaIntensity, 100.0f, boneMtx.Forward(), 1.0f, 30.0f, 35.0f, 3);

		if (isPlayer)
		{
			constexpr uint32_t CTaskAimGunOnFoot = 4;
			constexpr uint32_t CTaskAimGunVehicleDriveBy = 295;
			constexpr uint32_t CTaskAimGunBlindFire = 304;

			rage::aiTaskTree* taskTree = GetPedTaskTree(entity);
			rage::aiTask* task = nullptr;
			if ((task = taskTree->FindTaskByTypeActive(CTaskAimGunOnFoot)) ||
				(task = taskTree->FindTaskByTypeActive(CTaskAimGunVehicleDriveBy)) ||
				(task = taskTree->FindTaskByTypeActive(CTaskAimGunBlindFire)))
			{
				CTaskAimGun* aimTask = reinterpret_cast<CTaskAimGun*>(task);
				aimTask->DoShapeTest(entity, &startPos, &endPos, nullptr, nullptr, false);
			}
		}

		{
			static WorldProbe::CShapeTestResults* results = new WorldProbe::CShapeTestResults(1);

			results->AbortTest();
			WorldProbe::CShapeTestProbeDesc desc;
			desc.SetExcludeEntities(&This->m_ComponentObject, 1, 0);
			desc.SetResultsStructure(results);
			// TODO: find more appropriate shapetest flags
			desc.m_Flags1 = 256; // flags copied from game code
			desc.m_Flags2 = 0xE1134C2;
			desc.m_Start = startPos;
			desc.m_End = endPos;
			desc.m_84C = 8;

			if (info->DebugLines)
			{
				CScriptIM_DrawLine(startPos, endPos, 0xFF00FF00);
			}

			WorldProbe::GetShapeTestManager()->SubmitTest(desc, false);

			if (results->m_State == 4 && results->m_HitCount > 0)
			{
				for (int i = 0; i < results->m_HitCount; i++)
				{
					WorldProbe::CShapeTestHit* hit = &results->m_Hits[i];

					LaserBeam::DrawDot(hit->m_Position, hit->m_SurfaceNormal, dotColor);

					endPos = hit->m_Position;
				}

				results->AbortTest();
			}
		}

		{
			startPos = boneMtx.Position();
			const rage::Mat34V& camMtx = camBaseCamera::GetCurrentCamera()->GetTransform();

			// based on arbitrary axis billboards: http://nehe.gamedev.net/article/billboarding_how_to/18011/
			const rage::Vec3V center = (startPos + endPos) * 0.5f;
			const rage::Vec3V look = camMtx.Position() - center;
			const rage::Vec3V& up = (endPos - startPos).Normalized();
			const rage::Vec3V right = up.Cross(look).Normalized();

			if (info->DebugLines)
			{
				CScriptIM_DrawLine(startPos, endPos, 0xFF0000FF);
			}

			const float distance = (endPos - startPos).Length();
			const float endVisibility = info->MaxVisibility + ((info->MinVisibility - info->MaxVisibility) / info->BeamRange) * distance;

			LaserBeam::DrawBeam(info->BeamWidth, startPos, endPos, right, beamColor, info->MaxVisibility, endVisibility);
		}
	}
}

void LaserSight::InstallHooks()
{
	void** vtable = hook::get_absolute_address<void*>(hook::get_pattern("48 8D 05 ? ? ? ? C7 43 ? ? ? ? ? C6 43 50 00", 3));

	CWeaponComponentLaserSight_Process_orig = reinterpret_cast<decltype(CWeaponComponentLaserSight_Process_orig)>(vtable[3]);
	CWeaponComponentLaserSight_ProcessPostPreRender_orig = reinterpret_cast<decltype(CWeaponComponentLaserSight_ProcessPostPreRender_orig)>(vtable[4]);

	vtable[3] = CWeaponComponentLaserSight_Process_detour;
	vtable[4] = CWeaponComponentLaserSight_ProcessPostPreRender_detour;
}
