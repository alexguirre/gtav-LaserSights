#include "Addresses.h"
#include <Hooking.Patterns.h>
#include "Hooking.Helper.h"

void* const Addresses::CCoronas_Draw = hook::get_pattern("44 89 4C 24 ? 48 83 EC 28 0F 29 74 24 ?");

void* const Addresses::CCoronas_Instance =
	hook::get_absolute_address(hook::get_pattern("F3 41 0F 59 DD 48 8D 0D ? ? ? ? F3 0F 11 44 24 ?", 8));

void* const Addresses::CScriptIM_DrawLine = hook::get_pattern("48 8B DA 48 8B F9 E8 ? ? ? ? 84 C0 74 3F", -0xF);

void* const Addresses::fiAssetManager_PushFolder =
	hook::get_pattern("48 8B D9 48 8D 0D ? ? ? ? 48 8B FA E8 ? ? ? ? 80 3F 24", -0x12);

void* const Addresses::fiAssetManager_PopFolder = hook::get_pattern("FF 89 ? ? ? ? 48 8D 0D ? ? ? ? E9 ? ? ? ?");

void* const Addresses::fiAssetManager_Instance =
	hook::get_absolute_address(hook::get_pattern("48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8D 0D ? ? ? ? 33 D2 E8 ? ? ? ? 8B C8", 3));

void* const Addresses::grcEffect_LookupVar = 
	hook::get_absolute_address(hook::get_pattern("E8 ? ? ? ? 48 8D 15 ? ? ? ? 41 B0 01 89 46 20 48 8B 4F 08", 1));

void* const Addresses::grcEffect_LookupTechnique =
	hook::get_absolute_address(hook::get_pattern("E8 ? ? ? ? 48 8D 15 ? ? ? ? 45 8A C6 89 47 10 48 8B 47 08", 1));

void* const Addresses::grcEffect_SetVarCommon = hook::get_pattern("45 85 C0 74 4E 44 8B 54 24 ? 45 85 D2 7E 44");

void* const Addresses::grcEffect_SetVar = hook::get_pattern("45 33 D2 45 85 C0 74 47 41 8D 40 FF ");

void* const Addresses::grcInstanceData_LoadEffect = hook::get_pattern("48 83 EC 40 48 8B FA 48 8B D9 BA ? ? ? ?", -0x15);

void* const Addresses::grmShader_BeginDraw = hook::get_pattern("8B 05 ? ? ? ? 4C 63 DA 83 F8 FF 75 07 0F B6 05 ? ? ? ?");

void* const Addresses::grmShader_EndDraw =
	hook::get_absolute_address(hook::get_pattern("48 8B 4B 08 E8 ? ? ? ? 8B 0D ? ? ? ? E8 ? ? ? ? B2 01", 5));

void* const Addresses::grmShader_BeginPass = hook::get_pattern("4C 8B C1 48 8B 49 08 E9 ? ? ? ?");

void* const Addresses::grmShader_EndPass =
	hook::get_absolute_address(hook::get_pattern("E8 ? ? ? ? 49 83 C7 02 48 FF CD 0F 85 ? ? ? ?", 1));

void* const Addresses::grmShaderFactory_Instance =
	hook::get_absolute_address(hook::get_pattern("48 8B 0D ? ? ? ? 83 CE FF 48 8B 01 FF 50 08", 3));

void* const Addresses::GetTextureFromGraphicsTxd = hook::get_pattern("48 89 5C 24 ? 48 63 05 ? ? ? ? 48 8B 15 ? ? ? ? 8B D9");

void* const Addresses::grcDevice_CreateVertexDeclaration =
	hook::get_pattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 60 48 8B F1 48 63 CA");

void* const Addresses::grcDevice_SetVertexDeclaration = hook::get_pattern("48 89 0D ? ? ? ? 33 C0 C3");

void* const Addresses::grcDevice_BeginVertices =
	hook::get_pattern("48 83 EC 48 83 64 24 ? ? 48 83 64 24 ? ? 48 8D 44 24 ? 45 33 C9");

void* const Addresses::grcDevice_EndVertices =
	hook::get_absolute_address(hook::get_pattern("48 3B F2 0F 8C ? ? ? ? 33 C9 E8 ? ? ? ? E9 ? ? ? ? 8D 46 02 44 8D 24 7D", 12));

void* const Addresses::grcWorldIdentity = hook::get_pattern("48 8B C4 48 83 EC 68 0F 28 05 ? ? ? ? 0F 28 0D ? ? ? ?");

void* const Addresses::AddDrawCommandCallback = hook::get_pattern("E8 ? ? ? ? 33 D2 8D 4A 10 E8 ? ? ? ? 48 85 C0 74 04", -0x15);

void* const Addresses::CurrentCamera = hook::get_absolute_address(hook::get_pattern("48 8B 1D ? ? ? ? 0F 29 70 E8 48 8B F2", 3));

void* const Addresses::WorldProbe_CShapeTestResults_AbortTest = hook::get_pattern("48 89 5C 24 ? 57 48 83 EC 20 83 79 04 03 48 8B D9 75 10");

void* const Addresses::WorldProbe_CShapeTestDesc_SetResultsStructure = hook::get_pattern("48 8B D9 48 89 51 10 48 85 D2 74 34", -0xD);

void* const Addresses::WorldProbe_CShapeTestDesc_SetExcludeEntities = hook::get_absolute_address(hook::get_pattern("E8 ? ? ? ? 38 9D ? ? ? ? C7 85 ? ? ? ? ? ? ? ? 0F 95 C3 ", 1));

void* const Addresses::WorldProbe_CShapeTestManager_SubmitTest = hook::get_pattern("40 55 48 81 EC ? ? ? ? 48 8D 6C 24 ? 48 89 9D ? ? ? ?");

void* const Addresses::WorldProbe_GetShapeTestManager =
	hook::get_absolute_address(hook::get_pattern("0F 94 C3 E8 ? ? ? ? 44 8B C3 48 8B C8", 4));

void* const Addresses::atDataHash = hook::get_pattern("48 83 EC 28 E8 ? ? ? ? 8D 04 C0 8B C8");

void* const Addresses::aiTaskTree_FindTaskByTypeActive = hook::get_pattern("83 79 10 FF 74 20 48 63 41 10");

void* const Addresses::CWeaponComponentLaserSightInfo_parser_Data = hook::get_pattern("D2 D0 6A 4F 00 00 00 00");

// really fragile pattern, for example if size of CWeaponComponentInfo changes, the pattern will break
void* const Addresses::CWeaponComponentLaserSightInfo_parser_Register =
	hook::pattern("0F 85 AA 00 00 00 B9 58 00 00 00 E8 ? ? ? ? 4C 8D 0D ? ? ? ? 4C 8D 05 ? ? ? ? 48 8D 15 ? ? ? ? 48 8B C8 48").get(1).get<void>(-0xE);

void* const Addresses::CWeaponComponentLaserSightInfo_parser_Register_SizeOfConstant =
	reinterpret_cast<char*>(CWeaponComponentLaserSightInfo_parser_Register) + (0xE + 7);
