#include "LaserBeam.h"
#include <array>
#include <mutex>
#include <cstddef>
#include <cstdint>
#include <Hooking.Patterns.h>
#include "Hooking.Helper.h"
#include <spdlog/spdlog.h>
#include "grmShaderFactory.h"
#include "fiAssetManager.h"
#include "grcDevice.h"
#include "Addresses.h"
#include <MinHook.h>
#include "Matrix.h"
#include "Hashing.h"
#include <iterator>

static constexpr bool ShaderHotReloadEnabled{ true };

static struct LaserBeamGlobals
{
	rage::grmShader* Shader{ nullptr };
	struct
	{
		rage::grcEffectTechnique__ LaserBeam{ 0 };
	} Techniques{};
	struct
	{
		rage::grcEffectVar__ gTime{ 0 };
	} Vars{};
	struct
	{
		rage::grcVertexDeclaration* LaserBeam{ nullptr };
	} VertexDecls{};
} g_LaserBeam;

struct BeamDrawCall
{
	float m_HalfWidth{ 0.0f };
	rage::Vec3V m_From;
	rage::Vec3V m_To;
	rage::Vec3V m_RightVector;
	rage::Vec3V m_Color;
};
static std::array<BeamDrawCall, 256> g_BeamDrawCalls;
static size_t g_BeamDrawCallCount;
// copy of the draw calls to be used in the render thread
static std::array<BeamDrawCall, 256> g_BeamDrawCallsRender;
static size_t g_BeamDrawCallCountRender;

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
	}* gameTime = reinterpret_cast<fwTimeSet*>(Addresses::fwTimer_sm_gameTime);

	float time = gameTime->totalTimeMs * 0.001f;
	g_LaserBeam.Shader->m_Effect->SetVarCommon(g_LaserBeam.Shader, g_LaserBeam.Vars.gTime, &time, sizeof(float), 1);
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
	
	void* buffer = rage::grcDevice::BeginVertices(rage::grcDrawMode::TriangleStrip, 4, g_LaserBeam.VertexDecls.LaserBeam->m_VertexSize);
	for (size_t i = 0; i < VertexCount; i++)
	{
		SetLaserBeamVertex(buffer, i, p[i], drawCall.m_Color, uv[i]);
	}
	rage::grcDevice::EndVertices();
}

static void RenderBeams()
{
	if (g_BeamDrawCallCountRender > 0)
	{
		if (g_LaserBeam.Shader->BeginDraw(static_cast<rage::grmShader::eDrawType>(0), true, g_LaserBeam.Techniques.LaserBeam))
		{
			g_LaserBeam.Shader->BeginPass(0);

			rage::grcDevice::SetVertexDeclaration(g_LaserBeam.VertexDecls.LaserBeam);

			for (int i = 0; i < g_BeamDrawCallCountRender; i++)
			{
				RenderBeam(g_BeamDrawCallsRender[i]);
			}

			g_LaserBeam.Shader->EndPass();
			g_LaserBeam.Shader->EndDraw();
		}

		g_BeamDrawCallCountRender = 0;
	}
}

static void LoadShaderEffect()
{
	spdlog::debug("Loading shader effect...");
	if (g_LaserBeam.Shader->m_Effect)
	{
		spdlog::debug(" > Deleting existing effect");
		delete g_LaserBeam.Shader->m_Effect;
	}

	if constexpr (ShaderHotReloadEnabled)
	{
		rage::fiAssetManager::Instance()->PushFolder("shaders");
	}
	else
	{
		rage::fiAssetManager::Instance()->PushFolder("common:/shaders");
	}
	g_LaserBeam.Shader->LoadEffect("laserbeam");
	rage::fiAssetManager::Instance()->PopFolder();


	rage::grcEffect* effect = g_LaserBeam.Shader->m_Effect;
	spdlog::debug(" > Effect:{}", reinterpret_cast<void*>(effect));

	// lookup techniques
	g_LaserBeam.Techniques.LaserBeam = effect->LookupTechnique("LaserBeam");

	spdlog::debug(" > Techniques:");
	spdlog::debug("     LaserBeam:{}", g_LaserBeam.Techniques.LaserBeam);

	// lookup vars
	g_LaserBeam.Vars.gTime = effect->LookupVar("gTime");

	spdlog::debug(" > Vars:");
	spdlog::debug("     gTime:{}", g_LaserBeam.Vars.gTime);
}

static void Render()
{
	if constexpr (ShaderHotReloadEnabled)
	{
		static bool keyJustPressed = false;

		bool keyDown = GetAsyncKeyState(VK_F9) & 0x8000;
		if (!keyJustPressed && keyDown)
		{
			keyJustPressed = true;
			LoadShaderEffect();
		}
		else if (keyJustPressed && !keyDown)
		{
			keyJustPressed = false;
		}
	}

	if (g_BeamDrawCallCountRender > 0)
	{
		rage::grcWorldIdentity();
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
}

static void AddDrawCommandCallback(void(*cb)())
{
	using Fn = decltype(&AddDrawCommandCallback);
	reinterpret_cast<Fn>(Addresses::AddDrawCommandCallback)(cb);
}

static void(*sub_D63908_orig)(uint64_t a1);
static void sub_D63908_detour(uint64_t a1)
{
	// copy the draw calls to the render thread array
	memmove_s(g_BeamDrawCallsRender.data(), g_BeamDrawCallsRender.size() * sizeof(BeamDrawCall),
		g_BeamDrawCalls.data(), g_BeamDrawCallCount * sizeof(BeamDrawCall));
	g_BeamDrawCallCountRender = g_BeamDrawCallCount;
	g_BeamDrawCallCount = 0;

	AddDrawCommandCallback(Render);

	return sub_D63908_orig(a1);
}

void LaserBeam::InstallHooks()
{
	void* gtaRenderThreadGameInterface =
		hook::get_absolute_address(
			hook::get_absolute_address<char>(hook::get_pattern("E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 48 8B 5C 24 ? 48 83 C4 20 5F E9 ? ? ? ?", 11)) + 3
			);
	void** gtaRenderThreadGameInterfaceVTable = *reinterpret_cast<void***>(gtaRenderThreadGameInterface);

	CGtaRenderThreadGameInterface_RenderThreadInit_orig =
		reinterpret_cast<decltype(CGtaRenderThreadGameInterface_RenderThreadInit_orig)>(gtaRenderThreadGameInterfaceVTable[5]);

	gtaRenderThreadGameInterfaceVTable[5] = CGtaRenderThreadGameInterface_RenderThreadInit_detour;

	MH_CreateHook(hook::get_pattern("40 53 48 83 EC 20 8B D9 F6 C1 01 0F 84 ? ? ? ? E8 ? ? ? ? F6 80 ? ? ? ? ?"),
		sub_D63908_detour, reinterpret_cast<void**>(&sub_D63908_orig));
}

void LaserBeam::DrawBeam(float width, const rage::Vec3V& from, const rage::Vec3V& to, const rage::Vec3V& rightVector, const rage::Vec3V& color)
{
	if (g_BeamDrawCallCount < g_BeamDrawCalls.size())
	{
		g_BeamDrawCalls[g_BeamDrawCallCount] = { width * 0.5f, from, to, rightVector, color };
		g_BeamDrawCallCount++;
	}
}
