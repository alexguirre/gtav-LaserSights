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

		virtual ~fwEntity() = 0;
		virtual bool GetIsClassId(uint32_t classHash) = 0;
		virtual uint32_t GetClassId() = 0;
		virtual uint32_t GetBaseClassId() = 0;
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
