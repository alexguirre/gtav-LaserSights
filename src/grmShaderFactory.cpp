#include "grmShaderFactory.h"
#include "Addresses.h"
#include <intrin.h>

namespace rage
{
	grcEffect::~grcEffect()
	{
		using Fn = void(*)(grcEffect*);
		reinterpret_cast<Fn>(Addresses.grcEffect_dtor)(this);
	}

	grcEffectVar__ grcEffect::LookupVar(const char* name)
	{
		using Fn = grcEffectVar__(*)(grcEffect*, const char*);
		return reinterpret_cast<Fn>(Addresses.grcEffect_LookupVar)(this, name);
	}

	grcEffectTechnique__ grcEffect::LookupTechnique(const char* name)
	{
		using Fn = grcEffectTechnique__(*)(grcEffect*, const char*);
		return reinterpret_cast<Fn>(Addresses.grcEffect_LookupTechnique)(this, name);
	}

	void grcEffect::SetVarCommon(grcInstanceData* inst, grcEffectVar__ var, const void* data, int itemByteSize, int itemCount)
	{
		using Fn = void(*)(grcEffect*, grcInstanceData*, grcEffectVar__, const void*, int, int);
		reinterpret_cast<Fn>(Addresses.grcEffect_SetVarCommon)(this, inst, var, data, itemByteSize, itemCount);
	}

	void grcEffect::SetVar(grcInstanceData* inst, grcEffectVar__ var, grcTexture* texture)
	{
		using Fn = void(*)(grcEffect*, grcInstanceData*, grcEffectVar__, grcTexture*);
		reinterpret_cast<Fn>(Addresses.grcEffect_SetVar)(this, inst, var, texture);
	}

	void grcEffect::operator delete(void* ptr)
	{
		if (ptr)
		{
			uintptr_t allocator = *(uintptr_t*)(*(uintptr_t*)(__readgsqword(0x58u)) + 0xC8);
			(*(void(**)(uintptr_t, void*))(*(uintptr_t*)allocator + 0x20))(allocator, ptr); // rage::sysMemAllocator::Free
		}
	}

	bool grcInstanceData::LoadEffect(const char* name, void* tokenizer, bool a4)
	{
		using Fn = bool(*)(grcInstanceData*, const char*, void*, bool);
		return reinterpret_cast<Fn>(Addresses.grcInstanceData_LoadEffect)(this, name, tokenizer, a4);
	}

	int grmShader::BeginDraw(grmShader::eDrawType drawType, bool a3, grcEffectTechnique__ techniqueId)
	{
		using Fn = int(*)(grmShader*, grmShader::eDrawType, bool, uint32_t);
		return reinterpret_cast<Fn>(Addresses.grmShader_BeginDraw)(this, drawType, a3, techniqueId);
	}

	void grmShader::EndDraw()
	{
		using Fn = void(*)(grmShader*);
		reinterpret_cast<Fn>(Addresses.grmShader_EndDraw)(this);
	}

	void grmShader::BeginPass(int pass)
	{
		using Fn = void(*)(grmShader*, int);
		reinterpret_cast<Fn>(Addresses.grmShader_BeginPass)(this, pass);
	}

	void grmShader::EndPass()
	{
		using Fn = void(*)(grmShader*);
		reinterpret_cast<Fn>(Addresses.grmShader_EndPass)(this);
	}

	grmShaderFactory* grmShaderFactory::Instance()
	{
		return *reinterpret_cast<grmShaderFactory**>(Addresses.grmShaderFactory_Instance);
	}
}