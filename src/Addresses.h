#pragma once

class AddressManager
{
public:
	void* CCoronas_Draw = nullptr;
	void* CCoronas_Instance = nullptr;
	void* CScriptIM_DrawLine = nullptr;
	void* fiAssetManager_PushFolder = nullptr;
	void* fiAssetManager_PopFolder = nullptr;
	void* fiAssetManager_Instance = nullptr;
	void* grcEffect_dtor = nullptr;
	void* grcEffect_LookupVar = nullptr;
	void* grcEffect_LookupTechnique = nullptr;
	void* grcEffect_SetVarCommon = nullptr;
	void* grcEffect_SetVar = nullptr;
	void* grcInstanceData_LoadEffect = nullptr;
	void* grmShader_BeginDraw = nullptr;
	void* grmShader_EndDraw = nullptr;
	void* grmShader_BeginPass = nullptr;
	void* grmShader_EndPass = nullptr;
	void* grmShaderFactory_Instance = nullptr;
	void* grcDevice_CreateVertexDeclaration = nullptr;
	void* grcDevice_SetVertexDeclaration = nullptr;
	void* grcDevice_BeginVertices = nullptr;
	void* grcDevice_EndVertices = nullptr;
	void* grcCreateBlendState = nullptr;
	void* grcSetBlendState = nullptr;
	void* grcCreateDepthStencilState = nullptr;
	void* grcSetDepthStencilState = nullptr;
	void* grcWorldIdentity = nullptr;
	void* grcTextureFactory_Instance = nullptr;
	void* AddDrawCommandCallback = nullptr;
	void* AddDrawCommandCallbackInt32 = nullptr;
	void* CurrentCamera = nullptr;
	void* WorldProbe_CShapeTestResults_AbortTest = nullptr;
	void* WorldProbe_CShapeTestDesc_SetResultsStructure = nullptr;
	void* WorldProbe_CShapeTestDesc_SetExcludeEntities = nullptr;
	void* WorldProbe_CShapeTestManager_SubmitTest = nullptr;
	void* WorldProbe_GetShapeTestManager = nullptr;
	void* atDataHash = nullptr;
	void* aiTaskTree_FindTaskByTypeActive = nullptr;
	void* CWeaponComponentLaserSightInfo_parser_Data = nullptr;
	void* CWeaponComponentLaserSightInfo_parser_Register = nullptr;
	void* CWeaponComponentLaserSightInfo_parser_Register_SizeOfConstant = nullptr;
	void* CControlMgr_GetDefaultControl = nullptr;
	void* CControlMgr_GetIoValue = nullptr;
	void* ioValue_IsPressed = nullptr;
	void* IsNightVisionEnabled = nullptr;
	void* fwTimer_sm_gameTime = nullptr;
	void* CReplay_IsRecordingActive = nullptr;
	void* CPacketWeaponFlashLight_AddToRecording = nullptr;
	void* audWeaponAudioEntity_Instance = nullptr;
	void* PlayWeaponFlashLightToggleSound = nullptr;
	void* CGtaRenderThreadGameInterface_vftable = nullptr;
	void* sub_D63908 = nullptr;
	void* CWeaponComponentLaserSight_vftable = nullptr;
	void* CPacketObjectCreateBase_ctor_weaponComponentClassIdCheckHookLocation = nullptr;
	void* CPacketWeaponFlashLight_ReplayHandler_hookLocation = nullptr;
	void* EmbeddedFXCPatchLocation = nullptr;
	void* sub_55D038 = nullptr;
	void* drawDeferredVolumes = nullptr;
	void* addUpdateLightBuffersToRenderCommand = nullptr;
	void* resetSceneLights = nullptr;

	bool Init();
};

extern AddressManager Addresses;
