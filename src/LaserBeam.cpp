#include "LaserBeam.h"
#include <array>
#include <mutex>
#include <Hooking.Patterns.h>
#include "Hooking.Helper.h"
#include <spdlog/spdlog.h>
#include "grmShaderFactory.h"
#include "fiAssetManager.h"
#include "grcTexture.h"
#include "grcDevice.h"
#include "Addresses.h"
#include <MinHook.h>
#include "Matrix.h"
#include "Hashing.h"

static struct LaserBeamGlobals
{
	rage::grmShader* Shader = nullptr;
	struct
	{
		rage::grcEffectTechnique__ LaserBeam;
		rage::grcEffectTechnique__ LaserDot;
	} Techniques;
	struct
	{
		rage::grcEffectVar__ LaserTexture;
		rage::grcEffectVar__ gMaxDisplacement;
		rage::grcEffectVar__ gCameraDistanceAtMaxDisplacement;
		rage::grcEffectVar__ LaserVisibilityMinMax;
	} Vars;
	struct
	{
		rage::grcVertexDeclaration* LaserBeam;
		rage::grcVertexDeclaration* LaserDot;
	} VertexDecls;
	rage::grcTexture* LaserTexture = nullptr;
} g_LaserBeam;

struct BeamDrawCall
{
	rage::Vec3V m_From;
	rage::Vec3V m_To;
	rage::Vec3V m_RightVector;
};
static std::array<BeamDrawCall, 256> g_BeamDrawCalls;
static int g_BeamDrawCallCount;
// copy of the draw calls to be used in the render thread
static std::array<BeamDrawCall, 256> g_BeamDrawCallsRender;
static int g_BeamDrawCallCountRender;

struct DotDrawCall
{
	rage::Vec3V m_Position;
	rage::Vec3V m_Normal;
};
static std::array<DotDrawCall, 256> g_DotDrawCalls;
static int g_DotDrawCallCount;
// copy of the draw calls to be used in the render thread
static std::array<DotDrawCall, 256> g_DotDrawCallsRender;
static int g_DotDrawCallCountRender;

static rage::grcTexture* GetTextureFromGraphicsTxd(uint32_t nameHash)
{
	using Fn = rage::grcTexture* (*)(uint32_t);
	return reinterpret_cast<Fn>(Addresses::GetTextureFromGraphicsTxd)(nameHash);
}

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

static void SetLaserDotVertex(
	void* buffer, int index,
	const rage::Vec3V& position,
	const rage::Vec4V& color,
	const float texCoord0[2],
	const rage::Vec3V& normal)
{
	float* v = reinterpret_cast<float*>(reinterpret_cast<char*>(buffer) + g_LaserBeam.VertexDecls.LaserDot->m_VertexSize * index);
	v[0] = position.x;
	v[1] = position.y;
	v[2] = position.z;
	v[3] = color.x;
	v[4] = color.y;
	v[5] = color.z;
	v[6] = color.w;
	v[7] = texCoord0[0];
	v[8] = texCoord0[1];
	v[9] = 0.0f;
	v[10] = 0.0f;
	v[11] = 0.0f;
	v[12] = 0.0f;
	v[13] = normal.x;
	v[14] = normal.y;
	v[15] = normal.z;
}

static void SetShaderVars()
{
	g_LaserBeam.Shader->m_Effect->SetVar(g_LaserBeam.Shader, g_LaserBeam.Vars.LaserTexture, g_LaserBeam.LaserTexture);

	// TODO: shader variables customizeble per beam
	float LaserVisibilityMinMax[4] = { 0.2f, 100.0f, 0.0f, 0.0f };
	float gMaxDisplacement = 0.4f;
	float gCameraDistanceAtMaxDisplacement = 100.0f;

	g_LaserBeam.Shader->m_Effect->SetVarCommon(g_LaserBeam.Shader, g_LaserBeam.Vars.LaserVisibilityMinMax, LaserVisibilityMinMax, 16, 1);
	g_LaserBeam.Shader->m_Effect->SetVarCommon(g_LaserBeam.Shader, g_LaserBeam.Vars.gMaxDisplacement, &gMaxDisplacement, 4, 1);
	g_LaserBeam.Shader->m_Effect->SetVarCommon(g_LaserBeam.Shader, g_LaserBeam.Vars.gCameraDistanceAtMaxDisplacement, &gCameraDistanceAtMaxDisplacement, 4, 1);
}

static void RenderBeam(const BeamDrawCall& drawCall)
{
	constexpr float W = 0.015f;

	rage::Vec3V v0 = drawCall.m_From + drawCall.m_RightVector * W;
	rage::Vec3V v1 = drawCall.m_From - drawCall.m_RightVector * W;
	rage::Vec3V v2 = drawCall.m_To + drawCall.m_RightVector * W;
	rage::Vec3V v3 = drawCall.m_To - drawCall.m_RightVector * W;
	// TODO: color and other vertex variables customizeble per beam
	// TODO: decrease visibility of beam based on distance
	static const rage::Vec3V n(0.0f, 1.0f, 0.0f);
	static const rage::Vec4V color(4.9f, 0.24665f, 0.24665f, 0.3f);
	static const rage::Vec4V v0TexCoord0(0.0f, 1.0f, 0.0f, 0.0f);
	static const rage::Vec4V v0TexCoord2(0.015f, 1.0f, 0.0f, 0.0f);
	static const rage::Vec4V v1TexCoord0(0.0f, -1.0f, 0.0f, 0.0f);
	static const rage::Vec4V v1TexCoord2(0.015f, -1.0f, 0.0f, 0.0f);
	static const rage::Vec4V v2TexCoord0(243.5f, 1.0f, 0.0f, 0.0f);
	static const rage::Vec4V v2TexCoord2(0.01528f, 1.0f, 0.0f, 0.0f);
	static const rage::Vec4V v3TexCoord0(243.5f, -1.0f, 0.0f, 0.0f);
	static const rage::Vec4V v3TexCoord2(0.01528f, -1.0f, 0.0f, 0.0f);

	void* buffer = rage::grcDevice::BeginVertices(rage::grcDrawMode::TriangleStrip, 4, g_LaserBeam.VertexDecls.LaserBeam->m_VertexSize);
	SetLaserBeamVertex(buffer, 0, v0, n, v0TexCoord0, color, v0TexCoord2);
	SetLaserBeamVertex(buffer, 1, v1, n, v1TexCoord0, color, v1TexCoord2);
	SetLaserBeamVertex(buffer, 2, v2, n, v2TexCoord0, color, v2TexCoord2);
	SetLaserBeamVertex(buffer, 3, v3, n, v3TexCoord0, color, v3TexCoord2);
	rage::grcDevice::EndVertices();
}

static void RenderDot(const DotDrawCall& drawCall)
{
	static const rage::Vec4V color(1.0f, 0.0f, 0.0f, 0.2f);
	static const float v0TexCoord[2] = { 0.0f, 1.0f };
	static const float v1TexCoord[2] = { 0.0f, 0.0f };
	static const float v2TexCoord[2] = { 1.0f, 1.0f };
	static const float v3TexCoord[2] = { 1.0f, 0.0f };

	constexpr float dotScale = 0.1f;
	rage::Mat34V mtx = rage::Mat34V::FromNormal(drawCall.m_Normal);
	rage::Vec3V forwardScaled = mtx.Forward() * dotScale;
	rage::Vec3V rightScaled = mtx.Right() * dotScale;

	rage::Vec3V v0 = drawCall.m_Position - forwardScaled - rightScaled;
	rage::Vec3V v1 = drawCall.m_Position + forwardScaled - rightScaled;
	rage::Vec3V v2 = drawCall.m_Position - forwardScaled + rightScaled;
	rage::Vec3V v3 = drawCall.m_Position + forwardScaled + rightScaled;

	void* buffer = rage::grcDevice::BeginVertices(rage::grcDrawMode::TriangleStrip, 4, g_LaserBeam.VertexDecls.LaserDot->m_VertexSize);
	SetLaserDotVertex(buffer, 0, v0, color, v0TexCoord, drawCall.m_Normal);
	SetLaserDotVertex(buffer, 1, v1, color, v1TexCoord, drawCall.m_Normal);
	SetLaserDotVertex(buffer, 2, v2, color, v2TexCoord, drawCall.m_Normal);
	SetLaserDotVertex(buffer, 3, v3, color, v3TexCoord, drawCall.m_Normal);
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

static void RenderDots()
{
	if (g_DotDrawCallCountRender > 0)
	{
		if (g_LaserBeam.Shader->BeginDraw(static_cast<rage::grmShader::eDrawType>(0), true, g_LaserBeam.Techniques.LaserDot))
		{
			g_LaserBeam.Shader->BeginPass(0);

			rage::grcDevice::SetVertexDeclaration(g_LaserBeam.VertexDecls.LaserDot);

			for (int i = 0; i < g_DotDrawCallCountRender; i++)
			{
				RenderDot(g_DotDrawCallsRender[i]);
			}

			g_LaserBeam.Shader->EndPass();
			g_LaserBeam.Shader->EndDraw();
		}

		g_DotDrawCallCountRender = 0;
	}
}

static void Render()
{
	if (!g_LaserBeam.LaserTexture)
	{
		g_LaserBeam.LaserTexture = GetTextureFromGraphicsTxd(2716504251/*laser*/);
		spdlog::debug("LaserTexture:{}", reinterpret_cast<void*>(g_LaserBeam.LaserTexture));
	}

	if (g_BeamDrawCallCountRender > 0 || g_DotDrawCallCountRender > 0)
	{
		rage::grcWorldIdentity();
		SetShaderVars();

		RenderDots();
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
	g_LaserBeam.Techniques.LaserDot = effect->LookupTechnique("LaserDot");

	spdlog::debug("Techniques:");
	spdlog::debug("  LaserBeam:{}", g_LaserBeam.Techniques.LaserBeam);
	spdlog::debug("  LaserDot:{}", g_LaserBeam.Techniques.LaserDot);

	// lookup vars
	g_LaserBeam.Vars.LaserTexture = effect->LookupVar("LaserTexture");
	g_LaserBeam.Vars.gMaxDisplacement = effect->LookupVar("gMaxDisplacement");
	g_LaserBeam.Vars.gCameraDistanceAtMaxDisplacement = effect->LookupVar("gCameraDistanceAtMaxDisplacement");
	g_LaserBeam.Vars.LaserVisibilityMinMax = effect->LookupVar("LaserVisibilityMinMax");

	spdlog::debug("Vars:");
	spdlog::debug("  LaserTexture:{}", g_LaserBeam.Vars.LaserTexture);
	spdlog::debug("  gMaxDisplacement:{}", g_LaserBeam.Vars.gMaxDisplacement);
	spdlog::debug("  gCameraDistanceAtMaxDisplacement:{}", g_LaserBeam.Vars.gCameraDistanceAtMaxDisplacement);
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

	const rage::grcVertexElement laserDotVertexElements[] =
	{
		{ 0, rage::grcVertexElement::SemanticName::POSITION, 0, 12, rage::grcVertexElement::Format::R32G32B32_FLOAT, 0, 0 },
		{ 0, rage::grcVertexElement::SemanticName::COLOR,    0, 16, rage::grcVertexElement::Format::R32G32B32A32_FLOAT,  0, 0 },
		{ 0, rage::grcVertexElement::SemanticName::TEXCOORD, 0, 8,  rage::grcVertexElement::Format::R32G32_FLOAT,    0, 0 },
		{ 0, rage::grcVertexElement::SemanticName::TEXCOORD, 1, 8,  rage::grcVertexElement::Format::R32G32_FLOAT,    0, 0 },
		{ 0, rage::grcVertexElement::SemanticName::TEXCOORD, 2, 8,  rage::grcVertexElement::Format::R32G32_FLOAT,    0, 0 },
		{ 0, rage::grcVertexElement::SemanticName::NORMAL,   0, 12, rage::grcVertexElement::Format::R32G32B32_FLOAT, 0, 0 },
	};
	g_LaserBeam.VertexDecls.LaserDot = rage::grcDevice::CreateVertexDeclaration(laserDotVertexElements, ARRAYSIZE(laserDotVertexElements));

	spdlog::debug("Vertex Declarations:");
	spdlog::debug("  LaserBeam:{}", reinterpret_cast<void*>(g_LaserBeam.VertexDecls.LaserBeam));
	spdlog::debug("  LaserDot:{}", reinterpret_cast<void*>(g_LaserBeam.VertexDecls.LaserDot));

	// Fix up the constant buffers of the shader programs we use.
	//   In the `laserbeam` shader, the programs have the `rage_matrices` global buffer in register 0 and the `LaserParam`
	//   local buffer, in register 1, because it was ported from MP3.
	//   But in GTAV all shaders have `rage_matrices` in register 1 and since it is global only one instance is created which
	//   has the register field set to 1. When the game builds the cbuffers array for each program, only one of the buffers
	//   ends up assigned to it since both use the register 1.
	//   This workaround rebuilds the cbuffers arrays correctly, so `rage_matrices` is assigned to register 0 and `LaserParam` to register 1.
	{
		constexpr int VSLaserBeamIndex = 0;
		constexpr int VSLaserDotIndex = 1;
		constexpr int PSLaserBeamIndex = 1;
		constexpr int MaxCBuffers = 0x70 / sizeof(void*);

		// find relevant buffers
		char* laserParam = reinterpret_cast<char*>(effect->m_LocalBuffers.Items[0]); // LaserParam is the first and only item
		ID3D11Buffer* laserParamBuffer = *reinterpret_cast<ID3D11Buffer * *>(laserParam + 0x20);
		ID3D11Buffer* rageMatricesBuffer = effect->m_VertexPrograms.Items[VSLaserBeamIndex].m_CBuffers[1];

		rage::grcVertexProgram* vsLaserBeam = &effect->m_VertexPrograms.Items[VSLaserBeamIndex];
		rage::grcVertexProgram* vsLaserDot = &effect->m_VertexPrograms.Items[VSLaserDotIndex];
		rage::grcFragmentProgram* psLaserBeam = &effect->m_FragmentPrograms.Items[PSLaserBeamIndex];
		// PS_LaserDot not modified here because it doesn't use constant buffers

		// clear cbuffers array
		for (int i = 0; i < MaxCBuffers; i++)
		{
			vsLaserBeam->m_CBuffers[i] = nullptr;
			vsLaserDot->m_CBuffers[i] = nullptr;
			psLaserBeam->m_CBuffers[i] = nullptr;
		}

		// VS_LaserBeam
		//    cb0 = rage_matrices
		vsLaserBeam->m_CBuffers[0] = rageMatricesBuffer;
		vsLaserBeam->m_CBuffersStartSlot = 0;
		vsLaserBeam->m_CBuffersEndSlot = 0;

		// VS_LaserDot
		//    cb0 = rage_matrices
		//    cb1 = LaserParam
		vsLaserDot->m_CBuffers[0] = rageMatricesBuffer;
		vsLaserDot->m_CBuffers[1] = laserParamBuffer;
		vsLaserDot->m_CBuffersStartSlot = 0;
		vsLaserDot->m_CBuffersEndSlot = 1;

		// PS_LaserBeam
		//    cb1 = LaserParam
		psLaserBeam->m_CBuffers[1] = laserParamBuffer;
		psLaserBeam->m_CBuffersStartSlot = 1;
		psLaserBeam->m_CBuffersEndSlot = 1;

		// recalculate cbuffers hashes
		vsLaserBeam->m_CBuffersHash = rage::atDataHash(vsLaserBeam->m_CBuffers, 0x70);
		vsLaserDot->m_CBuffersHash = rage::atDataHash(vsLaserDot->m_CBuffers, 0x70);
		psLaserBeam->m_CBuffersHash = rage::atDataHash(psLaserBeam->m_CBuffers, 0x70);
	}
}

static void AddDrawCommandCallback(void(*cb)())
{
	using Fn = decltype(&AddDrawCommandCallback);
	reinterpret_cast<Fn>(Addresses::AddDrawCommandCallback)(cb);
}

static void(*DrawScriptWorldStuff_orig)(uint8_t n);
static void DrawScriptWorldStuff_detour(uint8_t n)
{
	DrawScriptWorldStuff_orig(n);

	// copy the draw calls to the render thread array
	memmove_s(g_BeamDrawCallsRender.data(), g_BeamDrawCallsRender.size() * sizeof(BeamDrawCall),
		g_BeamDrawCalls.data(), g_BeamDrawCallCount * sizeof(BeamDrawCall));
	g_BeamDrawCallCountRender = g_BeamDrawCallCount;
	g_BeamDrawCallCount = 0;

	memmove_s(g_DotDrawCallsRender.data(), g_DotDrawCallsRender.size() * sizeof(DotDrawCall),
		g_DotDrawCalls.data(), g_DotDrawCallCount * sizeof(DotDrawCall));
	g_DotDrawCallCountRender = g_DotDrawCallCount;
	g_DotDrawCallCount = 0;

	AddDrawCommandCallback(Render);
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

	MH_CreateHook(hook::get_pattern("40 53 48 83 EC 20 8B D9 F6 C1 01 74 18"), DrawScriptWorldStuff_detour, reinterpret_cast<void**>(&DrawScriptWorldStuff_orig));
}

void LaserBeam::DrawBeam(const rage::Vec3V& from, const rage::Vec3V& to, const rage::Vec3V& rightVector)
{
	if (g_BeamDrawCallCount < g_BeamDrawCalls.size())
	{
		g_BeamDrawCalls[g_BeamDrawCallCount] = { from, to, rightVector };
		g_BeamDrawCallCount++;
	}
}

void LaserBeam::DrawDot(const rage::Vec3V& position, const rage::Vec3V& normal)
{
	if (g_DotDrawCallCount < g_DotDrawCalls.size())
	{
		g_DotDrawCalls[g_DotDrawCallCount] = { position, normal };
		g_DotDrawCallCount++;
	}
}
