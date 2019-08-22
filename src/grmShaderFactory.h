#pragma once
#include <stdint.h>
#include "grcTexture.h"
#include "atArray.h"

namespace rage
{
	using grcEffectTechnique__ = uint32_t;
	using grcEffectVar__ = uint32_t;

	class grcInstanceData;

	class grcEffectTechniquePass
	{
	public:
		uint8_t m_VertexProgramIndex;
		uint8_t m_FragmentProgramIndex;
		uint8_t m_GeometryProgramIndex;
		uint8_t m_ComputeProgramIndex;
		uint8_t m_DomainProgramIndex;
		uint8_t m_HullProgramIndex;
		uint8_t m_RasterizerState;
		uint8_t m_DepthStencilState;
		uint8_t m_BlendState;
		uint8_t m_9;
		uint8_t m_A;
		uint8_t m_B;
	};

	class grcEffectTechnique
	{
	public:
		uint32_t m_NameHash;
		uint8_t padding_4[4];
		const char* m_Name;
		atArray<grcEffectTechniquePass> m_Passes;
	};

	class grcEffect
	{
	public:
		atArray<grcEffectTechnique> m_Techniques;
		uint8_t padding_10[0x10];
		void** m_LocalBuffers;
		// ...

		grcEffectVar__ LookupVar(const char* name);
		grcEffectTechnique__ LookupTechnique(const char* name);

		void SetVarCommon(grcInstanceData* inst, grcEffectVar__ var, const void* data, int itemByteSize, int itemCount);
		void SetVar(grcInstanceData* inst, grcEffectVar__ var, grcTexture* texture);
	};

	class grcInstanceData
	{
	public:
		uint8_t padding_0[0x8];
		grcEffect* m_Effect;
		// ...

		bool LoadEffect(const char* name, void* tokenizer = nullptr, bool a4 = true);
	};

	class grmShader : public grcInstanceData
	{
	public:
		enum class eDrawType : int {};

		int BeginDraw(eDrawType drawType, bool a3, grcEffectTechnique__ techniqueId);
		void EndDraw();
		void BeginPass(int pass);
		void EndPass();
	};

	class grmShaderFactory
	{
	public:
		virtual ~grmShaderFactory() = 0;
		virtual grmShader* Create() = 0;

		static grmShaderFactory* Instance();
	};
}