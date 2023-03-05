#include "Addresses.h"
#include <Hooking.Patterns.h>
#include <array>
#include <spdlog/spdlog.h>

AddressManager Addresses{};

/// `addr` should point to the RIP-relative offset, not to the first byte of the instruction.
template<typename T = void, int Length = 4>
inline T* GetRIPRelativeAddress(void* addr)
{
	char* from = reinterpret_cast<char*>(addr);
	int relAddr = *reinterpret_cast<int*>(addr);
	return reinterpret_cast<T*>(from + relAddr + Length);
}

static void LogNotFoundPattern(std::string_view storageName, std::string_view pattern, size_t numMatches, size_t expectedNumMatches)
{
	if (numMatches == 0)
	{
		spdlog::error(" > Pattern for '{}' ('{}') not found", storageName, pattern);
	}
	else
	{
		spdlog::error(" > Pattern for '{}' ('{}') only found {} matches, but expected {}", storageName, pattern, numMatches, expectedNumMatches);
	}
}

static bool FindNthImpl(void*& storage, std::string_view storageName, std::string_view pattern, size_t index, ptrdiff_t offset)
{
	const auto minSize = index + 1;
	auto p = hook::pattern(pattern).count_hint(minSize);
	if (p.size() < minSize)
	{
		LogNotFoundPattern(storageName, pattern, p.size(), minSize);
		return false;
	}

	storage = p.get(index).get<void>(offset);
	return true;
}

static bool FindImpl(void*& storage, std::string_view storageName, std::string_view pattern, ptrdiff_t offset)
{
	return FindNthImpl(storage, storageName, pattern, 0, offset);
}

template<int Length = 4>
static bool FindRIPImpl(void*& storage, std::string_view storageName, std::string_view pattern, ptrdiff_t offset)
{
	bool res = FindImpl(storage, storageName, pattern, offset);
	if (res)
	{
		storage = GetRIPRelativeAddress<void, Length>(storage);
	}
	return res;
}

#define FindNth(storage, pattern, index, offset) FindNthImpl(storage, #storage, pattern, index, offset)
#define Find(storage, pattern, offset) FindImpl(storage, #storage, pattern, offset)
#define FindRIP(storage, pattern, offset) FindRIPImpl(storage, #storage, pattern, offset)
#define FindRIPL(storage, pattern, offset, L) FindRIPImpl<L>(storage, #storage, pattern, offset)

bool AddressManager::Init()
{
	bool res = true;

	res &= Find(CCoronas_Draw, "44 89 4C 24 ? 48 83 EC 28 0F 29 74 24 ?", 0);

	res &= FindRIP(CCoronas_Instance, "F3 41 0F 59 DD 48 8D 0D ? ? ? ? F3 0F 11 44 24 ?", 8);

	res &= Find(CScriptIM_DrawLine, "48 8B DA 48 8B F9 E8 ? ? ? ? 84 C0 74 3F", -0xF);

	res &= Find(fiAssetManager_PushFolder, "48 8B D9 48 8D 0D ? ? ? ? 48 8B FA E8 ? ? ? ? 80 3F 24", -0x12);

	res &= Find(fiAssetManager_PopFolder, "FF 89 ? ? ? ? 48 8D 0D ? ? ? ? E9 ? ? ? ?", 0);

	res &= FindRIP(fiAssetManager_Instance, "48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8D 0D ? ? ? ? 33 D2 E8 ? ? ? ? 8B C8", 3);

	res &= Find(grcEffect_dtor, "48 8B F9 E8 ? ? ? ? 4C 63 0D ? ? ? ? 4D 8B D9", -0x18);

	res &= FindRIP(grcEffect_LookupVar, "E8 ? ? ? ? 48 8D 15 ? ? ? ? 41 B0 01 89 46 20 48 8B 4F 08", 1);

	res &= FindRIP(grcEffect_LookupTechnique, "E8 ? ? ? ? 48 8D 15 ? ? ? ? 45 8A C6 89 47 10 48 8B 47 08", 1);

	res &= Find(grcEffect_SetVarCommon, "45 85 C0 74 4E 44 8B 54 24 ? 45 85 D2 7E 44", 0);

	res &= Find(grcEffect_SetVar, "45 33 D2 45 85 C0 74 47 41 8D 40 FF ", 0);

	res &= Find(grcInstanceData_LoadEffect, "48 83 EC 40 48 8B FA 48 8B D9 BA ? ? ? ?", -0x15);

	res &= Find(grmShader_BeginDraw, "8B 05 ? ? ? ? 4C 63 DA 83 F8 FF 75 07 0F B6 05 ? ? ? ?", 0);

	res &= FindRIP(grmShader_EndDraw, "48 8B 4B 08 E8 ? ? ? ? 8B 0D ? ? ? ? E8 ? ? ? ? B2 01", 5);

	res &= Find(grmShader_BeginPass, "4C 8B C1 48 8B 49 08 E9 ? ? ? ?", 0);

	res &= FindRIP(grmShader_EndPass, "E8 ? ? ? ? 48 8B 0D ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 0F 28 75 60", 1);

	res &= FindRIP(grmShaderFactory_Instance, "48 8B 0D ? ? ? ? 83 CE FF 48 8B 01 FF 50 08", 3);

	res &= Find(grcDevice_CreateVertexDeclaration, "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 60 48 8B F1 48 63 CA", 0);

	res &= Find(grcDevice_SetVertexDeclaration, "48 89 0D ? ? ? ? 33 C0 C3", 0);

	res &= Find(grcDevice_BeginVertices, "48 83 EC 48 83 64 24 ? ? 48 83 64 24 ? ? 48 8D 44 24 ? 45 33 C9", 0);

	res &= FindRIP(grcDevice_EndVertices, "48 3B F2 0F 8C ? ? ? ? 33 C9 E8 ? ? ? ? E9 ? ? ? ? 8D 46 02 44 8D 24 7D", 12);

	res &= Find(grcCreateBlendState, "40 53 48 81 EC ? ? ? ? 33 D2 48 8B D9 48 8D 4C 24 ? 44 8D 42 78 E8", 0);

	res &= Find(grcSetBlendState, "49 83 C8 FF 83 CA FF E9", 0);

	res &= Find(grcCreateDepthStencilState, "40 55 48 8B EC 48 83 EC 70 8B 41 04 8B 11 89 45 CC 8B 41 08 89 55 C8", 0);

	res &= Find(grcSetDepthStencilState, "39 0D ? ? ? ? 8B 15 ? ? ? ? 65 48 8B 04 25", 0);

	res &= Find(grcWorldIdentity, "48 8B C4 48 83 EC 68 0F 28 05 ? ? ? ? 0F 28 0D", 0);

	res &= FindRIP(grcTextureFactory_Instance, "48 8B 1D ? ? ? ? 48 8B 10 48 8B 3B 48 8B C8 FF 92", 3);

	res &= Find(AddDrawCommandCallback, "E8 ? ? ? ? 33 D2 8D 4A 10 E8 ? ? ? ? 48 85 C0 74 04", -0x15);

	res &= FindRIP(AddDrawCommandCallbackInt32, "E8 ? ? ? ? 4C 8D 45 10 48 8D 0D ? ? ? ? 33 D2 C7 45", 1);

	res &= FindRIP(CurrentCamera, "48 8B 1D ? ? ? ? 0F 29 70 E8 48 8B F2", 3);

	res &= Find(WorldProbe_CShapeTestResults_AbortTest, "48 89 5C 24 ? 57 48 83 EC 20 83 79 04 03 48 8B D9 75 10", 0);

	res &= Find(WorldProbe_CShapeTestDesc_SetResultsStructure, "48 8B D9 48 89 51 10 48 85 D2 74 34", -0xD);

	res &= FindRIP(WorldProbe_CShapeTestDesc_SetExcludeEntities, "E8 ? ? ? ? 38 9D ? ? ? ? C7 85 ? ? ? ? ? ? ? ? 0F 95 C3 ", 1);

	res &= Find(WorldProbe_CShapeTestManager_SubmitTest, "40 55 48 81 EC ? ? ? ? 48 8D 6C 24 ? 48 89 9D ? ? ? ?", 0);

	res &= FindRIP(WorldProbe_GetShapeTestManager, "0F 94 C3 E8 ? ? ? ? 44 8B C3 48 8B C8", 4);

	res &= Find(atDataHash, "48 83 EC 28 E8 ? ? ? ? 8D 04 C0 8B C8", 0);

	res &= Find(aiTaskTree_FindTaskByTypeActive, "83 79 10 FF 74 20 48 63 41 10", 0);

	res &= Find(CWeaponComponentLaserSightInfo_parser_Data, "D2 D0 6A 4F 00 00 00 00", 0);

	// really fragile pattern, for example if size of CWeaponComponentInfo changes, the pattern will break
	res &= FindNth(CWeaponComponentLaserSightInfo_parser_Register,
		"0F 85 AA 00 00 00 B9 58 00 00 00 E8 ? ? ? ? 4C 8D 0D ? ? ? ? 4C 8D 05 ? ? ? ? 48 8D 15 ? ? ? ? 48 8B C8 48",
		1 /* index*/, -0xE /* offset */);

	if (CWeaponComponentLaserSightInfo_parser_Register)
	{
		CWeaponComponentLaserSightInfo_parser_Register_SizeOfConstant = (uint8_t*)CWeaponComponentLaserSightInfo_parser_Register + (0xE + 7);
	}

	res &= Find(CControlMgr_GetDefaultControl, "E8 ? ? ? ? 80 3D ? ? ? ? ? 48 8B D8 75 5A 80 3D ? ? ? ? ? 75 51 48 8D 0D ? ? ? ?", -0xD);

	res &= Find(CControlMgr_GetIoValue, "8D 82 ? ? ? ? 4C 8B C9 83 F8 1A 77 7F", 0);

	res &= Find(ioValue_IsPressed, "0F 29 74 24 ? 49 8B F8 48 8B F1 0F 28 F1 E8 ? ? ? ? 33 DB 84 C0 74 17", -0xF);

	res &= FindRIP(IsNightVisionEnabled, "44 88 35 ? ? ? ? 44 88 35 ? ? ? ? 44 89 35 ? ? ? ? 41 84 C7", 3);

	res &= FindRIP(fwTimer_sm_gameTime, "8B 05 ? ? ? ? 0F 5B C9 F3 0F 59 0D ? ? ? ? F3 0F 59 C8", 2);

	res &= Find(CReplay_IsRecordingActive, "8B 0D ? ? ? ? B8 ? ? ? ? 3B C8 74 0C 85 C9 75 11 38 05", 0);

	res &= FindRIP(CPacketWeaponFlashLight_AddToRecording, "4C 89 7D 88 E8 ? ? ? ? 4C 8D 9C 24 ? ? ? ? 49 8B 5B 48 41 0F 28 73", 5);

	{
		void* tempAddr = nullptr;
		res &= FindImpl(tempAddr, "PlayWeaponFlashLightToggleSound", "48 8D 0D ? ? ? ? 41 80 E0 01 E8 ? ? ? ? 48 85 FF 0F 84", 0);
		if (tempAddr)
		{
			audWeaponAudioEntity_Instance = GetRIPRelativeAddress((uint8_t*)tempAddr + 3);
			PlayWeaponFlashLightToggleSound = GetRIPRelativeAddress((uint8_t*)tempAddr + 12);
		}
	}

	{
		void* CGtaRenderThreadGameInterface = nullptr;
		res &= Find(CGtaRenderThreadGameInterface, "E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 48 8B 5C 24 ? 48 83 C4 20 5F E9 ? ? ? ?", 11);
		if (CGtaRenderThreadGameInterface)
		{
			CGtaRenderThreadGameInterface =
				GetRIPRelativeAddress(
					GetRIPRelativeAddress<char>(CGtaRenderThreadGameInterface) + 3
				);

			CGtaRenderThreadGameInterface_vftable = *reinterpret_cast<void**>(CGtaRenderThreadGameInterface);
		}
	}

	res &= Find(sub_D63908, "40 53 48 83 EC 20 8B D9 F6 C1 01 0F 84 ? ? ? ? E8 ? ? ? ? F6 80 ? ? ? ? ?", 0);

	res &= FindRIP(CWeaponComponentLaserSight_vftable, "48 8D 05 ? ? ? ? C7 43 ? ? ? ? ? C6 43 50 00", 3);

	res &= Find(CPacketObjectCreateBase_ctor_weaponComponentClassIdCheckHookLocation, "83 F8 02 74 16 48 8B 86 ? ? ? ? 48 8B 48 08", 0);

	res &= Find(CPacketWeaponFlashLight_ReplayHandler_hookLocation, "4C 8B 82 ? ? ? ? 4D 85 C0 74 33 8A 51 1D 41 8A 48 49", 0);
	
	res &= Find(EmbeddedFXCPatchLocation, "BA ? ? ? ? E8 ? ? ? ? 84 C0 74 1B 4C 8D 4C 24", 5);

	res &= Find(sub_55D038, "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 40 8A D9 E8 ? ? ? ? 0F 28 05", 0);

	res &= Find(drawDeferredVolumes, "48 83 EC 30 83 3D ? ? ? ? ? 44 8A F9 0F 84 ? ? ? ? 48 8B 0D ? ? ? ? 44 8B 05", -0x14);

	res &= Find(addUpdateLightBuffersToRenderCommand, "48 83 EC 38 F6 05 ? ? ? ? ? 75 30 E8 ? ? ? ? 48 85 C0 74 1F", 0);

	res &= Find(resetSceneLights, "8B 0D ? ? ? ? 8B 15 ? ? ? ? 33 ED FF C1 4C 8D 35 ? ? ? ? 89 2D", -0x19);

	res &= FindRIP(DepthBufferPreAlpha, "48 8B 05 ? ? ? ? 33 D2 44 39 35 ? ? ? ? 76 2D 4C 8D 8D", 3);
	
	res &= Find(fwEntity_GetBoneIndex, "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8B 01 83 4C 24", 0);
	
	res &= Find(DRAW_SPOT_LIGHT, "E8 ? ? ? ? F3 0F 10 85 ? ? ? ? 0F 2E 05 ? ? ? ? 75 08 F3 0F 10 05 ? ? ? ? 48 8D 4C 24 ? 45 33 C0 33 D2", -0x1CB);

	res &= FindRIPL(ShaderQuality, "83 3D ? ? ? ? 00 75 09 48 8D 1D ? ? ? ? EB 14 48 8D 1D", 2, 5);

	return res;
}
