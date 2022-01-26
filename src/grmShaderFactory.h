#pragma once
#include <stdint.h>
#include <d3d11.h>
#include "atArray.h"

namespace rage
{
	class grcTexture;

	using grcEffectTechnique__ = uint32_t;
	using grcEffectVar__ = uint32_t;

	class grcInstanceData;

	class grcProgram
	{
	public:
		const char* m_Name;
		atArray<uint32_t> m_LocalVariablesIds;
		ID3D11Buffer** m_CBuffers;
		uint32_t m_CBuffersHash;
		uint8_t m_CBuffersStartSlot;
		uint8_t m_CBuffersEndSlot;
		uint8_t padding_2E[0x20A];

		virtual ~grcProgram() = 0;
	};
	static_assert(sizeof(grcProgram) == 0x238);

	class grcVertexProgram : public grcProgram
	{
	public:
		uint8_t padding_238[0x10];
	};
	static_assert(sizeof(grcVertexProgram) == 0x248);

	class grcFragmentProgram : public grcProgram
	{
	};
	static_assert(sizeof(grcFragmentProgram) == 0x238);

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
		atArray<void*> m_LocalBuffers; // atArray<grcBufferD3D11*>
		atArray<grcVertexProgram> m_VertexPrograms;
		atArray<grcFragmentProgram> m_FragmentPrograms;
		// ...

		~grcEffect();

		grcEffectVar__ LookupVar(const char* name);
		grcEffectTechnique__ LookupTechnique(const char* name);

		void SetVarCommon(grcInstanceData* inst, grcEffectVar__ var, const void* data, int itemByteSize, int itemCount);
		void SetVar(grcInstanceData* inst, grcEffectVar__ var, grcTexture* texture);

		static void operator delete(void* ptr);
	};

	class grcInstanceData
	{
	public:
		uint8_t padding_0[0x8];
		grcEffect* m_Effect;
		// ...

		bool LoadEffect(const char* name, void* tokenizer = nullptr, bool returnDefaultIfFailed = false);
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