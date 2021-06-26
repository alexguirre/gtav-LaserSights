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
#include "Replay.h"
#include <filesystem>

static struct Config
{
	uint32_t ShapeTestFlags1 = 0x100;
	uint32_t ShapeTestFlags2 = 0xE1134C2;
} g_Config;

static constexpr bool ConfigHotReloadEnabled{ true };

static void LoadConfig(const char* path)
{
	spdlog::debug("Loading config {}", path);

	std::string strShapeTestFlags1(512, '\0'), strShapeTestFlags2(512, '\0');
	strShapeTestFlags1.resize(GetPrivateProfileString("Config", "ShapeTestFlags1", "0", strShapeTestFlags1.data(), strShapeTestFlags1.size(), path));
	strShapeTestFlags1.resize(GetPrivateProfileString("Config", "ShapeTestFlags2", "0", strShapeTestFlags2.data(), strShapeTestFlags2.size(), path));
	spdlog::debug(" > strShapeTestFlags1 = {}", strShapeTestFlags1);
	spdlog::debug(" > strShapeTestFlags2 = {}", strShapeTestFlags2);

	g_Config.ShapeTestFlags1 = std::stoul(strShapeTestFlags1, nullptr, 0);
	g_Config.ShapeTestFlags2 = std::stoul(strShapeTestFlags2, nullptr, 0);
	spdlog::debug(" > g_Config.ShapeTestFlags1 = 0x{:X}", g_Config.ShapeTestFlags1);
	spdlog::debug(" > g_Config.ShapeTestFlags2 = 0x{:X}", g_Config.ShapeTestFlags2);
}

static DWORD ConfigFileWatcher(LPVOID)
{
	char dirPath[MAX_PATH], filePath[MAX_PATH];
	GetFullPathName(".\\", MAX_PATH, dirPath, NULL);
	GetFullPathName(".\\LaserSightWeaponComponent.ini", MAX_PATH, filePath, NULL);
	HANDLE handle = FindFirstChangeNotification(dirPath, FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);
	if (handle == INVALID_HANDLE_VALUE)
	{
		spdlog::debug("Failed to create config file watcher");
		return 0;
	}

	auto prevWriteTime = std::filesystem::last_write_time(filePath);

	while (true)
	{
		switch (WaitForSingleObject(handle, INFINITE))
		{
		case WAIT_OBJECT_0:
			const auto newWriteTime = std::filesystem::last_write_time(filePath);
			if (prevWriteTime != newWriteTime)
			{
				LoadConfig(filePath);
				prevWriteTime = newWriteTime;
			}

			if (!FindNextChangeNotification(handle))
			{
				spdlog::debug("FindNextChangeNotification failed");
				FindCloseChangeNotification(handle);
				return 0;
			}
			break;
		}
	}
}

static void StartConfigFileWatcher()
{
	HANDLE h = CreateThread(NULL, 0, &ConfigFileWatcher, NULL, 0, NULL);
	CloseHandle(h);
}

static void CScriptIM_DrawLine(const rage::Vec3V& start, const rage::Vec3V& end, uint32_t color)
{
	reinterpret_cast<decltype(&CScriptIM_DrawLine)>(Addresses::CScriptIM_DrawLine)(start, end, color);
}

static rage::aiTaskTree* GetPedTaskTree(rage::fwEntity* ped)
{
	// TODO: get offsets from patterns
	constexpr int IntelligenceOffset = 0x10C0;
	constexpr int TaskManagerOffset = 0x370; // b2245
	constexpr int TaskTreeOffset = 0x0;

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

	CControl* const c = ((CControl * (*)(bool))Addresses::CControlMgr_GetDefaultControl)(true);
	ioValue* const v = ((ioValue * (*)(CControl*, int))Addresses::CControlMgr_GetIoValue)(c, INPUT_CONTEXT);

	constexpr struct ioValue_ReadOptions { float f1; uint32_t f2; } Options{ 0.0f, 1 };
	constexpr float Threshold{ 0.5f };
	return ((bool(*)(ioValue*, float, const ioValue_ReadOptions&))Addresses::ioValue_IsPressed)(v, Threshold, Options);
}

static bool IsNightVisionEnabled()
{
	return *reinterpret_cast<bool*>(Addresses::IsNightVisionEnabled);
}

static void PlayToggleSound(void* weapon, bool isOn)
{
	reinterpret_cast<void(*)(void*, void*, bool)>(Addresses::PlayWeaponFlashLightToggleSound)(Addresses::audWeaponAudioEntity_Instance, weapon, isOn);
}

static bool IsLaserVisible(CWeaponComponentLaserSight* sight)
{
	if (sight->m_IsOff)
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

static void(*CWeaponComponentLaserSight_Process_orig)(CWeaponComponentLaserSight* This, rage::fwEntity* entity);
static void CWeaponComponentLaserSight_Process_detour(CWeaponComponentLaserSight* This, rage::fwEntity* entity)
{
	if (IsContextPressed())
	{
		This->m_IsOff = !This->m_IsOff;
		PlayToggleSound(This->m_OwnerWeapon, !This->m_IsOff);
	}
}

static void(*CWeaponComponentLaserSight_ProcessPostPreRender_orig)(CWeaponComponentLaserSight* This, rage::fwEntity* entity);
static void CWeaponComponentLaserSight_ProcessPostPreRender_detour(CWeaponComponentLaserSight* This, rage::fwEntity* entity)
{
	if (This->m_OwnerWeapon)
	{
		Replay::RecordLaserSightState(GetWeaponObject(This), !This->m_IsOff);
	}

	if (!IsLaserVisible(This))
	{
		return;
	}

	if (This->m_OwnerWeapon && This->m_ComponentObject && This->m_LaserSightBoneIndex != -1)
	{
		// TODO: replace hardcoded offset
		constexpr int PlayerInfoOffset = 0x10C8; // b2245
		const bool isPlayer = entity && *reinterpret_cast<void**>((reinterpret_cast<uint8_t*>(entity) + PlayerInfoOffset));

		const auto* info = reinterpret_cast<ExtendedWeaponComponentLaserSightInfo*>(This->m_ComponentInfo);

		rage::Mat34V boneMtx;
		This->m_ComponentObject->GetGlobalMtx(This->m_LaserSightBoneIndex, &boneMtx);

		const rage::Vec3V origStartPos = boneMtx.Position();
		const rage::Vec3V origEndPos = origStartPos + boneMtx.Forward() * info->BeamRange;

		rage::Vec3V startPos = origStartPos, endPos = origEndPos;

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
				rage::Vec3V aimStartPos = origStartPos, aimEndPos = origEndPos;
				aimTask->DoShapeTest(entity, &aimStartPos, &aimEndPos, nullptr, nullptr, false);

				const rage::Vec3V origDir = (origEndPos - origStartPos).Normalized();
				const rage::Vec3V aimDir = (aimEndPos - aimStartPos).Normalized();

				const float angleCos = origDir.Dot(aimDir);
				const float angleDegrees = acosf(angleCos) * 180.0f / 3.1415926f;
				if (angleDegrees < 17.5f)
				{
					// only use aiming direction if it is close enough to the component direction
					// to avoid visual issues in first person/rolling/etc.
					startPos = aimStartPos;
					endPos = aimEndPos;
				}
			}
		}

		{
			static WorldProbe::CShapeTestResults results{ 4 };

			results.AbortTest();
			WorldProbe::CShapeTestProbeDesc desc;
			const rage::fwEntity* excludeEntities[]{ This->m_ComponentObject, GetWeaponObject(This) };
			desc.SetExcludeEntities(excludeEntities, ARRAYSIZE(excludeEntities), 0);
			desc.SetResultsStructure(&results);
			// TODO: find more appropriate shapetest flags
			desc.m_Flags1 = g_Config.ShapeTestFlags1;
			desc.m_Flags2 = g_Config.ShapeTestFlags2;
			desc.m_Start = startPos;
			desc.m_End = endPos;
			desc.m_84C = 8;

			if (info->DebugLines)
			{
				CScriptIM_DrawLine(startPos, endPos, 0xFF00FF00);
			}

			WorldProbe::GetShapeTestManager()->SubmitTest(desc, false);

			if (results.m_State == 4 && results.m_HitCount > 0)
			{
				for (int i = 0; i < results.m_HitCount; i++)
				{
					WorldProbe::CShapeTestHit* hit = &results.m_Hits[i];

					endPos = hit->m_Position;

					CCoronas::Instance()->Draw(hit->m_Position, info->CoronaSize * 0.25f, info->CoronaColor, info->CoronaIntensity * 1.25f, 100.0f, hit->m_SurfaceNormal, 1.0f, 65.0f, 85.0f, 3);
				}

				results.AbortTest();
			}
		}

		{
			startPos = boneMtx.Position();
			const rage::Mat34V& camMtx = camBaseCamera::GetCurrentCamera()->GetTransform();

			// based on arbitrary axis billboards: http://nehe.gamedev.net/article/billboarding_how_to/18011/
			const rage::Vec3V center = (startPos + endPos) * 0.5f;
			const rage::Vec3V look = camMtx.Position() - center;
			const rage::Vec3V up = (endPos - startPos).Normalized();
			const rage::Vec3V right = up.Cross(look).Normalized();

			if (info->DebugLines)
			{
				CScriptIM_DrawLine(startPos, endPos, 0xFF0000FF);
			}

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

void LaserSight::InstallHooks()
{
	void** vtable = hook::get_absolute_address<void*>(hook::get_pattern("48 8D 05 ? ? ? ? C7 43 ? ? ? ? ? C6 43 50 00", 3));

	CWeaponComponentLaserSight_Process_orig = reinterpret_cast<decltype(CWeaponComponentLaserSight_Process_orig)>(vtable[3]);
	CWeaponComponentLaserSight_ProcessPostPreRender_orig = reinterpret_cast<decltype(CWeaponComponentLaserSight_ProcessPostPreRender_orig)>(vtable[4]);
	CWeaponComponentLaserSight_ApplyAccuracyModifier_orig = reinterpret_cast<decltype(CWeaponComponentLaserSight_ApplyAccuracyModifier_orig)>(vtable[5]);

	vtable[3] = CWeaponComponentLaserSight_Process_detour;
	vtable[4] = CWeaponComponentLaserSight_ProcessPostPreRender_detour;
	vtable[5] = CWeaponComponentLaserSight_ApplyAccuracyModifier_detour;

	StartConfigFileWatcher();
}
