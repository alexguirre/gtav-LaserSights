#include "LaserBeam.h"
#include <array>
#include <mutex>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <fileapi.h>
#include <spdlog/spdlog.h>
#include "grmShaderFactory.h"
#include "fiAssetManager.h"
#include "fiDevice.h"
#include "grcDevice.h"
#include "grcTextureFactory.h"
#include "Addresses.h"
#include <MinHook.h>
#include "Matrix.h"
#include "Hashing.h"
#include "Resources.h"
#include <d3d9.h>

static constexpr bool ShaderHotReloadEnabled
{
#if _DEBUG
	true
#else
	false
#endif
};
static bool ReloadShaders{ false };

static DWORD ShadersFileWatcher(LPVOID)
{
	char path[MAX_PATH];
	GetFullPathName(".\\shaders\\", MAX_PATH, path, NULL);
	HANDLE handle = FindFirstChangeNotification(path, TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE);
	if (handle == INVALID_HANDLE_VALUE)
	{
		spdlog::debug("Failed to create shaders file watcher");
		return 0;
	}

	while (true)
	{
		switch (WaitForSingleObject(handle, INFINITE))
		{
		case WAIT_OBJECT_0:
			ReloadShaders = true;
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

static void StartShadersFileWatcher()
{
	HANDLE h = CreateThread(NULL, 0, &ShadersFileWatcher, NULL, 0, NULL);
	CloseHandle(h);
}

static struct LaserBeamGlobals
{
	rage::grcBlendStateHandle BlendState{ 0 };
	rage::grcDepthStencilStateHandle DepthStencilState{ 0 };
	rage::grmShader* Shader{ nullptr };
	struct
	{
		rage::grcEffectTechnique__ LaserBeam{ 0 };
	} Techniques{};
	struct
	{
		rage::grcEffectVar__ gTime{ 0 };
		rage::grcEffectVar__ LaserNoise{ 0 };
		rage::grcEffectVar__ DepthBuffer{ 0 };
		rage::grcEffectVar__ DepthBufferPreAlpha{ 0 };
	} Vars{};
	struct
	{
		rage::grcVertexDeclaration* LaserBeam{ nullptr };
	} VertexDecls{};
	rage::grcTexture* LaserNoiseTexture{ nullptr };
} g_LaserBeam;

struct BeamDrawCall
{
	float m_HalfWidth{ 0.0f };
	rage::Vec3V m_From;
	rage::Vec3V m_To;
	rage::Vec3V m_RightVector;
	rage::Vec3V m_Color;
};
struct BeamDrawCallsBuffer
{
	std::array<BeamDrawCall, 256> m_DrawCalls;
	size_t m_NumDrawCalls{ 0 };
};
static std::array<BeamDrawCallsBuffer, 2> g_BeamDrawCallBuffers;
static std::array<bool, 2> g_BeamDrawCallBufferSentToRender;
static uint32_t g_BeamDrawCallBuffersIdx{ 0 };
// copy of the draw calls to be used in the render thread
static BeamDrawCallsBuffer g_BeamDrawCallBufferToRender;

static HMODULE g_hModule;

static std::string MakeResourceMemoryFileName(int resourceId, const char* name)
{
	auto hResource = FindResourceA(g_hModule, MAKEINTRESOURCEA(resourceId), "RAW");
	auto hMemory = LoadResource(g_hModule, hResource);
	auto resourceSize = SizeofResource(g_hModule, hResource);
	auto resourcePtr = LockResource(hMemory);

	char fileName[256];
	rage::fiDevice::MakeMemoryFileName(fileName, std::size(fileName), resourcePtr, resourceSize, false, name);
	return fileName;
}

static void SetLaserBeamVertex(
	void* buffer, size_t index,
	const rage::Vec3V& position, const rage::Vec3V& color, const float texcoord[2])
{
	float* v = reinterpret_cast<float*>(reinterpret_cast<char*>(buffer) + g_LaserBeam.VertexDecls.LaserBeam->m_VertexSize * index);
	v[0] = position.x;
	v[1] = position.y;
	v[2] = position.z;
	v[3] = color.x;
	v[4] = color.y;
	v[5] = color.z;
	v[6] = texcoord[0];
	v[7] = texcoord[1];
}

static void UpdateTime()
{
	struct fwTimeSet
	{
		uint32_t totalTimeMs;
		// ...
	}* gameTime = reinterpret_cast<fwTimeSet*>(Addresses.fwTimer_sm_gameTime);

	float time = gameTime->totalTimeMs * 0.001f;
	g_LaserBeam.Shader->m_Effect->SetVarCommon(g_LaserBeam.Shader, g_LaserBeam.Vars.gTime, &time, sizeof(float), 1);
}

static void BindNoiseTexture()
{
	if (g_LaserBeam.Shader->m_Effect && g_LaserBeam.LaserNoiseTexture)
	{
		g_LaserBeam.Shader->m_Effect->SetVar(g_LaserBeam.Shader, g_LaserBeam.Vars.LaserNoise, g_LaserBeam.LaserNoiseTexture);
	}
}

static void RenderBeam(const BeamDrawCall& drawCall)
{
	constexpr size_t VertexCount{ 4 };
	const rage::Vec3V p[VertexCount]
	{
		drawCall.m_From + drawCall.m_RightVector * drawCall.m_HalfWidth,
		drawCall.m_From - drawCall.m_RightVector * drawCall.m_HalfWidth,
		drawCall.m_To + drawCall.m_RightVector * drawCall.m_HalfWidth,
		drawCall.m_To - drawCall.m_RightVector * drawCall.m_HalfWidth,
	};
	const float length = (drawCall.m_To - drawCall.m_From).Length();
	const float uv[VertexCount][2]
	{
		{ 0.0f, 1.0f },
		{ 0.0f, 0.0f },
		{ length, 1.0f },
		{ length, 0.0f },
	};
	
	void* buffer = rage::grcDevice::BeginVertices(rage::grcDrawMode::TriangleStrip, VertexCount, g_LaserBeam.VertexDecls.LaserBeam->m_VertexSize);
	for (size_t i = 0; i < VertexCount; i++)
	{
		SetLaserBeamVertex(buffer, i, p[i], drawCall.m_Color, uv[i]);
	}
	rage::grcDevice::EndVertices();
}

static void RenderBeams()
{
	if (g_BeamDrawCallBufferToRender.m_NumDrawCalls > 0)
	{
		rage::grcSetBlendState(g_LaserBeam.BlendState);
		//rage::grcSetDepthStencilState(g_LaserBeam.DepthStencilState);
		if (g_LaserBeam.Shader->BeginDraw(static_cast<rage::grmShader::eDrawType>(0), true, g_LaserBeam.Techniques.LaserBeam))
		{
			g_LaserBeam.Shader->BeginPass(0);

			rage::grcDevice::SetVertexDeclaration(g_LaserBeam.VertexDecls.LaserBeam);

			for (int i = 0; i < g_BeamDrawCallBufferToRender.m_NumDrawCalls; i++)
			{
				RenderBeam(g_BeamDrawCallBufferToRender.m_DrawCalls[i]);
			}

			g_LaserBeam.Shader->EndPass();
			g_LaserBeam.Shader->EndDraw();
		}
	}
}

static void LoadNoiseTexture()
{
	if (g_LaserBeam.LaserNoiseTexture)
	{
		// TODO: destroy noise texture
		g_LaserBeam.LaserNoiseTexture = nullptr;
	}

	if constexpr (ShaderHotReloadEnabled)
	{
		auto path = "cfx:/plugins/shaders/laser_noise.dds";
		spdlog::debug("Loading external noise texture from '{}' (hot-reload)...", path);
		g_LaserBeam.LaserNoiseTexture = rage::grcTextureFactory::Instance()->Create(path);
	}
	else // Load embedded DDS
	{
		auto path = MakeResourceMemoryFileName(LASERSIGHTS_RES_ID_LASER_NOISE_DDS, "laser_noise.dds");
		spdlog::debug("Loading embedded noise texture from '{}'...", path);
		g_LaserBeam.LaserNoiseTexture = rage::grcTextureFactory::Instance()->Create(path.c_str());
	}
	spdlog::debug(" > LaserNoiseTexture:{}", reinterpret_cast<void*>(g_LaserBeam.LaserNoiseTexture));
}

static void LoadShaderEffect()
{
	if (g_LaserBeam.BlendState == 0)
	{
		spdlog::debug("Creating blend state...");
		D3D11_BLEND_DESC blendDesc{};
		blendDesc.AlphaToCoverageEnable = false;
		blendDesc.IndependentBlendEnable = false;
		blendDesc.RenderTarget[0] = {
			true,
			D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD,	// color
			D3D11_BLEND_ZERO, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD,	// alpha
			D3D11_COLOR_WRITE_ENABLE_ALL
		};
		g_LaserBeam.BlendState = rage::grcCreateBlendState(blendDesc);

		D3D11_DEPTH_STENCIL_DESC depthStencilDesc{};
		depthStencilDesc.BackFace = {
			D3D11_STENCIL_OP_ZERO,					// StencilFailOp
			D3D11_STENCIL_OP_ZERO,					// StencilDepthFailOp
			D3D11_STENCIL_OP_ZERO,					// StencilPassOp
			D3D11_COMPARISON_GREATER_EQUAL,	// StencilFunc
		};
		depthStencilDesc.DepthFunc = D3D11_COMPARISON_GREATER;
		depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		depthStencilDesc.FrontFace = {
			D3D11_STENCIL_OP_ZERO,					// StencilFailOp
			D3D11_STENCIL_OP_ZERO,					// StencilDepthFailOp
			D3D11_STENCIL_OP_ZERO,					// StencilPassOp
			D3D11_COMPARISON_GREATER_EQUAL,	// StencilFunc
		};
		depthStencilDesc.DepthEnable = true;
		depthStencilDesc.StencilEnable = false;
		depthStencilDesc.StencilReadMask = 0;
		depthStencilDesc.StencilWriteMask = 0;
		g_LaserBeam.DepthStencilState = rage::grcCreateDepthStencilState(depthStencilDesc);
	}

	spdlog::debug("Loading shader effect...");
	if (g_LaserBeam.Shader->m_Effect)
	{
		spdlog::debug(" > Deleting existing effect");
		delete g_LaserBeam.Shader->m_Effect;
	}

	if constexpr (ShaderHotReloadEnabled)
	{
		rage::fiAssetManager::Instance()->PushFolder("cfx:/plugins/shaders");
		spdlog::debug("Loading external shader FXC from shaders directory in game root (hot-reload)...");
		g_LaserBeam.Shader->LoadEffect("laserbeam");
		rage::fiAssetManager::Instance()->PopFolder();
	}
	else // Load embedded FXC
	{
		// Patch call to rage::fiAssetManager::InsertPathElement that prefixes the shader name with
		// the correct directory (win32_40_final/, win32_40_lq_final/ or win32_nvstereo_final/).
		// This doesn't work with the memory file names used to load the embedded FXC,
		// which end up looking like win32_40_final/memory:$123ABC...
		auto addr = reinterpret_cast<uint8_t*>(Addresses.EmbeddedFXCPatchLocation);
		std::array<uint8_t, 5> savedCallInstruction;
		std::memcpy(savedCallInstruction.data(), addr, savedCallInstruction.size());
		addr[0] = 0xB0; // mov al, 1
		addr[1] = 0x01;
		addr[2] = 0x90; // nop
		addr[3] = 0x90; // nop
		addr[4] = 0x90; // nop
		FlushInstructionCache(GetCurrentProcess(), addr, savedCallInstruction.size());

		auto path = MakeResourceMemoryFileName(LASERSIGHTS_RES_ID_LASERBEAM_FXC, "laserbeam.fxc");
		spdlog::debug("Loading embedded shader FXC from '{}'...", path);
		g_LaserBeam.Shader->LoadEffect(path.c_str());

		// Restore call to rage::fiAssetManager::InsertPathElement
		std::memcpy(addr, savedCallInstruction.data(), savedCallInstruction.size());
		FlushInstructionCache(GetCurrentProcess(), addr, savedCallInstruction.size());
	}

	rage::grcEffect* effect = g_LaserBeam.Shader->m_Effect;
	spdlog::debug(" > Effect:{}", reinterpret_cast<void*>(effect));
	if (!effect)
	{
		spdlog::debug("Loading shader failed...");
		spdlog::default_logger()->flush();
		return;
	}

	// lookup techniques
	g_LaserBeam.Techniques.LaserBeam = effect->LookupTechnique("LaserBeam");

	spdlog::debug(" > Techniques:");
	spdlog::debug("     LaserBeam:{}", g_LaserBeam.Techniques.LaserBeam);

	// lookup vars
	g_LaserBeam.Vars.gTime = effect->LookupVar("gTime");
	g_LaserBeam.Vars.LaserNoise = effect->LookupVar("LaserNoise");
	g_LaserBeam.Vars.DepthBuffer = effect->LookupVar("DepthBuffer");
	g_LaserBeam.Vars.DepthBufferPreAlpha = effect->LookupVar("DepthBufferPreAlpha");

	spdlog::debug(" > Vars:");
	spdlog::debug("     gTime:{}", g_LaserBeam.Vars.gTime);
	spdlog::debug("     LaserNoise:{}", g_LaserBeam.Vars.LaserNoise);
	spdlog::debug("     DepthBuffer:{}", g_LaserBeam.Vars.DepthBuffer);
	spdlog::debug("     DepthBufferPreAlpha:{}", g_LaserBeam.Vars.DepthBufferPreAlpha);

	ReloadShaders = false;
}

static void Render(rage::grcTexture* depthBuffer)
{
	if constexpr (ShaderHotReloadEnabled)
	{
		static bool keyJustPressed = false;

		bool keyDown = GetAsyncKeyState(VK_F9) & 0x8000;
		if (!keyJustPressed && keyDown)
		{
			keyJustPressed = true;
			ReloadShaders = true;
		}
		else if (keyJustPressed && !keyDown)
		{
			keyJustPressed = false;
		}

		if (ReloadShaders)
		{
			LoadShaderEffect();
			LoadNoiseTexture();
			BindNoiseTexture();
		}
	}

	if (g_BeamDrawCallBufferToRender.m_NumDrawCalls > 0)
	{
		if (g_LaserBeam.Shader->m_Effect && depthBuffer)
		{
			g_LaserBeam.Shader->m_Effect->SetVar(g_LaserBeam.Shader, g_LaserBeam.Vars.DepthBuffer, depthBuffer);
		}
		if (g_LaserBeam.Shader->m_Effect && Addresses.DepthBufferPreAlpha && *(void**)Addresses.DepthBufferPreAlpha)
		{
			g_LaserBeam.Shader->m_Effect->SetVar(g_LaserBeam.Shader, g_LaserBeam.Vars.DepthBufferPreAlpha, *(rage::grcTexture**)Addresses.DepthBufferPreAlpha);
		}

		//rage::grcWorldIdentity();
		UpdateTime();

		RenderBeams();
	}
}

static void(*CGtaRenderThreadGameInterface_RenderThreadInit_orig)(void* This);
static void CGtaRenderThreadGameInterface_RenderThreadInit_detour(void* This)
{
	CGtaRenderThreadGameInterface_RenderThreadInit_orig(This);

	spdlog::debug("RenderThreadInit");

	g_LaserBeam.Shader = rage::grmShaderFactory::Instance()->Create();
	spdlog::debug("LaserBeam Shader:{}", reinterpret_cast<void*>(g_LaserBeam.Shader));
	LoadShaderEffect();
	LoadNoiseTexture();
	BindNoiseTexture();


	// create vertex declarations
	const rage::grcVertexElement laserBeamVertexElements[] =
	{
		/* InputSlot         SemanticName         SemanticIndex ByteSize        Format                     InputSlotClass InstanceDataStepRate */
		{ 0, rage::grcVertexElement::SemanticName::POSITION, 0, 12, rage::grcVertexElement::Format::R32G32B32_FLOAT,    0, 0 },
		{ 0, rage::grcVertexElement::SemanticName::COLOR,    0, 12, rage::grcVertexElement::Format::R32G32B32_FLOAT,    0, 0 },
		{ 0, rage::grcVertexElement::SemanticName::TEXCOORD, 0, 8,  rage::grcVertexElement::Format::R32G32_FLOAT,       0, 0 },
	};
	g_LaserBeam.VertexDecls.LaserBeam = rage::grcDevice::CreateVertexDeclaration(laserBeamVertexElements, ARRAYSIZE(laserBeamVertexElements));

	spdlog::debug("Vertex Declarations:");
	spdlog::debug("  LaserBeam:{}", reinterpret_cast<void*>(g_LaserBeam.VertexDecls.LaserBeam));

	if constexpr (ShaderHotReloadEnabled)
	{
		//StartShadersFileWatcher();
	}
}

static void AddDrawCommandCallback(void(*cb)())
{
	using Fn = decltype(&AddDrawCommandCallback);
	reinterpret_cast<Fn>(Addresses.AddDrawCommandCallback)(cb);
}

static void AddDrawCommandCallbackInt32(void(*cb)(uint32_t arg), uint64_t unk, uint32_t& arg)
{
	using Fn = decltype(&AddDrawCommandCallbackInt32);
	reinterpret_cast<Fn>(Addresses.AddDrawCommandCallbackInt32)(cb, unk, arg);
}

static void(*drawDeferredVolumes_orig)(uint8_t a, rage::grcTexture* depthBuffer);
static void drawDeferredVolumes_detour(uint8_t a, rage::grcTexture* depthBuffer)
{
	D3DPERF_BeginEvent(D3DCOLOR_RGBA(255, 255, 255, 255), L"drawDeferredVolumes");

	drawDeferredVolumes_orig(a, depthBuffer);

	D3DPERF_BeginEvent(D3DCOLOR_RGBA(255, 0, 0, 255), L"Render Laser Sights");
	Render(depthBuffer);
	D3DPERF_EndEvent();
	
	D3DPERF_EndEvent();
}

static void(*addUpdateLightBuffersToRenderCommand_orig)();
static void addUpdateLightBuffersToRenderCommand_detour()
{
	addUpdateLightBuffersToRenderCommand_orig();
	
	AddDrawCommandCallbackInt32([](uint32_t bufferIndex)
	{
		if (!g_BeamDrawCallBufferSentToRender[bufferIndex])
		{
			auto& currBuffer = g_BeamDrawCallBuffers[bufferIndex];
			std::copy_n(currBuffer.m_DrawCalls.begin(), currBuffer.m_NumDrawCalls, g_BeamDrawCallBufferToRender.m_DrawCalls.begin());
			g_BeamDrawCallBufferToRender.m_NumDrawCalls = currBuffer.m_NumDrawCalls;
			g_BeamDrawCallBufferSentToRender[bufferIndex] = true;
		}
	}, 0, g_BeamDrawCallBuffersIdx);
}

static void(*resetSceneLights_orig)();
static void resetSceneLights_detour()
{
	resetSceneLights_orig();
	
	g_BeamDrawCallBuffersIdx = (g_BeamDrawCallBuffersIdx + 1) % g_BeamDrawCallBuffers.size();
	g_BeamDrawCallBuffers[g_BeamDrawCallBuffersIdx].m_NumDrawCalls = 0;
	g_BeamDrawCallBufferSentToRender[g_BeamDrawCallBuffersIdx] = false;
}

static bool InstallHooks()
{
	void** gtaRenderThreadGameInterfaceVTable = (void**)Addresses.CGtaRenderThreadGameInterface_vftable;

	CGtaRenderThreadGameInterface_RenderThreadInit_orig = (decltype(CGtaRenderThreadGameInterface_RenderThreadInit_orig))gtaRenderThreadGameInterfaceVTable[5];

	gtaRenderThreadGameInterfaceVTable[5] = CGtaRenderThreadGameInterface_RenderThreadInit_detour;

	const auto res1 = MH_CreateHook(Addresses.drawDeferredVolumes, drawDeferredVolumes_detour, (void**)&drawDeferredVolumes_orig);
	const auto res2 = MH_CreateHook(Addresses.addUpdateLightBuffersToRenderCommand, addUpdateLightBuffersToRenderCommand_detour, (void**)&addUpdateLightBuffersToRenderCommand_orig);
	const auto res3 = MH_CreateHook(Addresses.resetSceneLights, resetSceneLights_detour, (void**)&resetSceneLights_orig);

	return res1 == MH_OK && res2 == MH_OK && res3 == MH_OK;
}

bool LaserBeam::Init(HMODULE hModule)
{
	g_hModule = hModule;
	return InstallHooks();
}

void LaserBeam::DrawBeam(float width, const rage::Vec3V& from, const rage::Vec3V& to, const rage::Vec3V& rightVector, const rage::Vec3V& color)
{
	auto& currBuffer = g_BeamDrawCallBuffers[g_BeamDrawCallBuffersIdx];
	if (currBuffer.m_NumDrawCalls < currBuffer.m_DrawCalls.size())
	{
		currBuffer.m_DrawCalls[currBuffer.m_NumDrawCalls] = { width * 0.5f, from, to, rightVector, color };
		currBuffer.m_NumDrawCalls++;
	}
}
