#pragma once
#include <stdint.h>
#include "Matrix.h"

namespace rage
{
	class fwEntity
	{
	public:
		uint8_t m_10[0x50];
		Mat34V Transform;

		virtual void* RTTI_Get() = 0;
		virtual void* RTTI_Get2() = 0;
		virtual uint32_t RTTI_GetName() = 0;
		virtual void* RTTI_DynamicCast(void* targetType) = 0;
		virtual bool RTTI_Is(void* targetType) = 0;
		virtual bool RTTI_Is2(void* targetType) = 0;
		virtual ~fwEntity() = 0;
		virtual void f_3() = 0;
		virtual void f_4() = 0;
		virtual void f_5() = 0;
		virtual void f_6() = 0;
		virtual void f_7() = 0;
		virtual void f_8() = 0;
		virtual Mat34V* GetGlobalMtx(uint32_t boneIndex, Mat34V* outMtx) = 0;

		uint32_t GetBoneIndex(uint16_t boneId) const;
	};
}
