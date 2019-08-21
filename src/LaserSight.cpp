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

		rage::Vec3V startPos = { boneMtx.m[3][0], boneMtx.m[3][1], boneMtx.m[3][2] };
		rage::Vec3V endPos = *This->m_RaycastHitPosition;

		CScriptIM_DrawLine(startPos, endPos, 0xFF0000FF);

		// TODO: corona doesn't use the direction of the bone
		CCoronas::Instance()->Draw(startPos, This->m_ComponentInfo->CoronaSize, 0xFFFF0000, This->m_ComponentInfo->CoronaIntensity, 100.0f, { 0.0f, 1.0f, 0.0f }, 1.0f, 5.5f, 8.5f, 3);
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