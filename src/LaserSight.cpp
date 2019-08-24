#include "LaserSight.h"
#include <Hooking.Patterns.h>
#include "CWeaponComponentLaserSight.h"
#include "CCoronas.h"
#include <spdlog/spdlog.h>
#include "Hooking.Helper.h"
#include "Addresses.h"
#include "LaserBeam.h"
#include "camBaseCamera.h"
#include "WorldProbe.h"

static void CScriptIM_DrawLine(const rage::Vec3V& start, const rage::Vec3V& end, uint32_t color)
{
	reinterpret_cast<decltype(&CScriptIM_DrawLine)>(Addresses::CScriptIM_DrawLine)(start, end, color);
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
		rage::Mat34V boneMtx;
		This->m_ComponentObject->GetGlobalMtx(This->m_LaserSightBoneIndex, &boneMtx);

		rage::Vec3V startPos = boneMtx.Position();
		rage::Vec3V endPos = *This->m_RaycastHitPosition;
		rage::Vec3V forwardEndPos = startPos + boneMtx.Forward() * 100.0f;

		CScriptIM_DrawLine(startPos, endPos, 0xFF0000FF);
		CScriptIM_DrawLine(startPos, forwardEndPos, 0xFFFF0000);

		CCoronas::Instance()->Draw(startPos, This->m_ComponentInfo->CoronaSize, 0xFFFF0000, This->m_ComponentInfo->CoronaIntensity, 100.0f, boneMtx.Forward(), 1.0f, 30.0f, 35.0f, 3);
		
		{
			static WorldProbe::CShapeTestResults* results = new WorldProbe::CShapeTestResults(1);

			results->AbortTest();
			WorldProbe::CShapeTestProbeDesc desc;
			desc.SetResultsStructure(results);
			desc.m_Flags1 = 256; // flags copied from game code
			desc.m_Flags2 = 0xE1134C2;
			desc.m_Start = startPos;
			desc.m_End = forwardEndPos;
			desc.m_84C = 8;

			WorldProbe::GetShapeTestManager()->SubmitTest(desc, false);

			if (results->m_State == 4 && results->m_HitCount > 0)
			{
				for (int i = 0; i < results->m_HitCount; i++)
				{
					WorldProbe::CShapeTestHit* hit = &results->m_Hits[i];

					LaserBeam::DrawDot(hit->m_Position, hit->m_SurfaceNormal);

					forwardEndPos = hit->m_Position;
				}

				results->AbortTest();
			}
		}

		{
			const rage::Mat34V& camMtx = camBaseCamera::GetCurrentCamera()->GetTransform();

			// based on arbitrary axis billboards: http://nehe.gamedev.net/article/billboarding_how_to/18011/
			const rage::Vec3V center = (startPos + forwardEndPos) * 0.5f;
			const rage::Vec3V look = camMtx.Position() - center;
			const rage::Vec3V& up = boneMtx.Forward();
			const rage::Vec3V right = up.Cross(look).Normalized();

			LaserBeam::DrawBeam(startPos, forwardEndPos, right);
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
