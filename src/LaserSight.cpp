#include "LaserSight.h"
#include <Hooking.Patterns.h>
#include "CWeaponComponentLaserSight.h"
#include "CCoronas.h"
#include <spdlog/spdlog.h>

static void CScriptIM_DrawLine(const rage::Vec3V& start, const rage::Vec3V& end, uint32_t color)
{
	static void* addr = hook::get_pattern("48 8B DA 48 8B F9 E8 ? ? ? ? 84 C0 74 3F", -0xF);

	((decltype(&CScriptIM_DrawLine))addr)(start, end, color);
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

	if (This->m_OwnerWeapon && This->m_ComponentObject/* && This->m_HasRaycastHit*/)
	{
		rage::Mat34V boneMtx;
		This->m_ComponentObject->GetGlobalMtx(This->m_LaserSightBoneIndex, &boneMtx);

		rage::Vec3V startPos = boneMtx.Position();
		rage::Vec3V endPos = *This->m_RaycastHitPosition;
		rage::Vec3V forwardEndPos = startPos + boneMtx.Forward() * 25.0f;

		CScriptIM_DrawLine(startPos, endPos, 0xFF0000FF);
		CScriptIM_DrawLine(startPos, forwardEndPos, 0xFFFF0000);

		CCoronas::Instance()->Draw(startPos, This->m_ComponentInfo->CoronaSize, 0xFFFF0000, This->m_ComponentInfo->CoronaIntensity, 100.0f, boneMtx.Forward(), 1.0f, 30.0f, 35.0f, 3);
	}
}

void LaserSight::InstallHooks()
{
	auto vtableAddr = hook::get_pattern<char>("48 8D 05 ? ? ? ? C7 43 ? ? ? ? ? C6 43 50 00", 3);
	void** vtable = (void**)(vtableAddr + *(int*)vtableAddr + 4);

	CWeaponComponentLaserSight_Process_orig = reinterpret_cast<decltype(CWeaponComponentLaserSight_Process_orig)>(vtable[3]);
	CWeaponComponentLaserSight_ProcessPostPreRender_orig = reinterpret_cast<decltype(CWeaponComponentLaserSight_ProcessPostPreRender_orig)>(vtable[4]);

	vtable[3] = CWeaponComponentLaserSight_Process_detour;
	vtable[4] = CWeaponComponentLaserSight_ProcessPostPreRender_detour;
}