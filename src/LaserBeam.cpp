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

static struct LaserBeamGlobals
{
	rage::grmShader* Shader{ nullptr };
	struct
	{
		rage::grcEffectTechnique__ LaserBeam{ 0 };
	} Techniques{};
	struct
	{
		rage::grcEffectVar__ LaserVisibilityMinMax{ 0 };
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
	float m_FromVisibility{ 0.0f };
	float m_ToVisibility{ 0.0f };
};
static std::array<BeamDrawCall, 256> g_BeamDrawCalls;
static size_t g_BeamDrawCallCount;
// copy of the draw calls to be used in the render thread
static std::array<BeamDrawCall, 256> g_BeamDrawCallsRender;
static size_t g_BeamDrawCallCountRender;

static void SetLaserBeamVertex(
	void* buffer, int index,
	const rage::Vec3V& position, const rage::Vec3V& normal,
	const rage::Vec4V& texCoord0, const rage::Vec4V& texCoord1Color, const rage::Vec4V& texCoord2)
{
	float* v = reinterpret_cast<float*>(reinterpret_cast<char*>(buffer) + g_LaserBeam.VertexDecls.LaserBeam->m_VertexSize * index);
	v[0] = position.x;
	v[1] = position.y;
	v[2] = position.z;
	v[3] = normal.x;
	v[4] = normal.y;
	v[5] = normal.z;
	v[6] = texCoord0.x;
	v[7] = texCoord0.y;
	v[8] = texCoord0.z;
	v[9] = texCoord0.w;
	v[10] = texCoord1Color.x;
	v[11] = texCoord1Color.y;
	v[12] = texCoord1Color.z;
	v[13] = texCoord1Color.w;
	v[14] = texCoord2.x;
	v[15] = texCoord2.y;
	v[16] = texCoord2.z;
	v[17] = texCoord2.w;
}

static void SetShaderLaserVisibilityMinMax(float min, float max)
{
	float values[2] = { min, max };
	g_LaserBeam.Shader->m_Effect->SetVarCommon(g_LaserBeam.Shader, g_LaserBeam.Vars.LaserVisibilityMinMax, values, 8, 1);
}

static void SetDefaultShaderVars()
{
	SetShaderLaserVisibilityMinMax(0.2f, 100.0f);
}

static void RenderBeam(const BeamDrawCall& drawCall)
{
	const rage::Vec3V v0 = drawCall.m_From + drawCall.m_RightVector * drawCall.m_HalfWidth;
	const rage::Vec3V v1 = drawCall.m_From - drawCall.m_RightVector * drawCall.m_HalfWidth;
	const rage::Vec3V v2 = drawCall.m_To + drawCall.m_RightVector * drawCall.m_HalfWidth;
	const rage::Vec3V v3 = drawCall.m_To - drawCall.m_RightVector * drawCall.m_HalfWidth;

	static const rage::Vec3V n(0.0f, 1.0f, 0.0f);
	static const rage::Vec4V v0TexCoord0(0.0f, 1.0f, 0.0f, 0.0f);
	static const rage::Vec4V v0TexCoord2(0.015f, 1.0f, 0.0f, 0.0f);
	static const rage::Vec4V v1TexCoord0(0.0f, -1.0f, 0.0f, 0.0f);
	static const rage::Vec4V v1TexCoord2(0.015f, -1.0f, 0.0f, 0.0f);
	static const rage::Vec4V v2TexCoord0(243.5f, 1.0f, 0.0f, 0.0f);
	static const rage::Vec4V v2TexCoord2(0.01528f, 1.0f, 0.0f, 0.0f);
	static const rage::Vec4V v3TexCoord0(243.5f, -1.0f, 0.0f, 0.0f);
	static const rage::Vec4V v3TexCoord2(0.01528f, -1.0f, 0.0f, 0.0f);

	const rage::Vec4V colorOrig(drawCall.m_Color.x, drawCall.m_Color.y, drawCall.m_Color.z, drawCall.m_FromVisibility);
	const rage::Vec4V colorEnd(drawCall.m_Color.x, drawCall.m_Color.y, drawCall.m_Color.z, drawCall.m_ToVisibility);

	SetShaderLaserVisibilityMinMax(drawCall.m_ToVisibility, drawCall.m_FromVisibility);

	void* buffer = rage::grcDevice::BeginVertices(rage::grcDrawMode::TriangleStrip, 4, g_LaserBeam.VertexDecls.LaserBeam->m_VertexSize);
	SetLaserBeamVertex(buffer, 0, v0, n, v0TexCoord0, colorOrig, v0TexCoord2);
	SetLaserBeamVertex(buffer, 1, v1, n, v1TexCoord0, colorOrig, v1TexCoord2);
	SetLaserBeamVertex(buffer, 2, v2, n, v2TexCoord0, colorEnd, v2TexCoord2);
	SetLaserBeamVertex(buffer, 3, v3, n, v3TexCoord0, colorEnd, v3TexCoord2);
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

static void Render()
{
	if (g_BeamDrawCallCountRender > 0)
	{
		rage::grcWorldIdentity();
		SetDefaultShaderVars();

		RenderBeams();
	}
}

static void(*CGtaRenderThreadGameInterface_RenderThreadInit_orig)(void* This);
static void CGtaRenderThreadGameInterface_RenderThreadInit_detour(void* This)
{
	CGtaRenderThreadGameInterface_RenderThreadInit_orig(This);

	spdlog::debug("RenderThreadInit");

	// load shader
	rage::fiAssetManager::Instance()->PushFolder("common:/shaders");
	g_LaserBeam.Shader = rage::grmShaderFactory::Instance()->Create();
	g_LaserBeam.Shader->LoadEffect("laserbeam");
	rage::fiAssetManager::Instance()->PopFolder();

	spdlog::debug("LaserBeam Shader:{}", reinterpret_cast<void*>(g_LaserBeam.Shader));

	rage::grcEffect* effect = g_LaserBeam.Shader->m_Effect;

	// lookup techniques
	g_LaserBeam.Techniques.LaserBeam = effect->LookupTechnique("LaserBeam");

	spdlog::debug("Techniques:");
	spdlog::debug("  LaserBeam:{}", g_LaserBeam.Techniques.LaserBeam);

	// lookup vars
	g_LaserBeam.Vars.LaserVisibilityMinMax = effect->LookupVar("LaserVisibilityMinMax");

	spdlog::debug("Vars:");
	spdlog::debug("  LaserVisibilityMinMax:{}", g_LaserBeam.Vars.LaserVisibilityMinMax);

	// create vertex declarations
	const rage::grcVertexElement laserBeamVertexElements[] =
	{
		/* InputSlot         SemanticName         SemanticIndex ByteSize        Format                     InputSlotClass InstanceDataStepRate */
		{ 0, rage::grcVertexElement::SemanticName::POSITION, 0, 12, rage::grcVertexElement::Format::R32G32B32_FLOAT,    0, 0 },
		{ 0, rage::grcVertexElement::SemanticName::NORMAL,   0, 12, rage::grcVertexElement::Format::R32G32B32_FLOAT,    0, 0 },
		{ 0, rage::grcVertexElement::SemanticName::TEXCOORD, 0, 16, rage::grcVertexElement::Format::R32G32B32A32_FLOAT, 0, 0 },
		{ 0, rage::grcVertexElement::SemanticName::TEXCOORD, 1, 16, rage::grcVertexElement::Format::R32G32B32A32_FLOAT, 0, 0 },
		{ 0, rage::grcVertexElement::SemanticName::TEXCOORD, 2, 16, rage::grcVertexElement::Format::R32G32B32A32_FLOAT, 0, 0 },
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

void LaserBeam::DrawBeam(float width, const rage::Vec3V& from, const rage::Vec3V& to, const rage::Vec3V& rightVector, const rage::Vec3V& color, float fromVisibility, float toVisibility)
{
	if (g_BeamDrawCallCount < g_BeamDrawCalls.size())
	{
		g_BeamDrawCalls[g_BeamDrawCallCount] = { width * 0.5f, from, to, rightVector, color, fromVisibility, toVisibility };
		g_BeamDrawCallCount++;
	}
}
