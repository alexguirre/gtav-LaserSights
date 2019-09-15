#pragma once
#include <stdint.h>

namespace rage
{
	class aiTask
	{
	public:
		virtual ~aiTask() = 0;
		virtual uint32_t GetTypeIndex() = 0;
		virtual void f_2() = 0;
		virtual void f_3() = 0;
		virtual void f_4() = 0;
		virtual void f_5() = 0;
		virtual void f_6() = 0;
		virtual void f_7() = 0;
		virtual void f_8() = 0;
		virtual void f_9() = 0;
		virtual void f_10() = 0;
		virtual void f_11() = 0;
		virtual void f_12() = 0;
		virtual void f_13() = 0;
		virtual void f_14() = 0;
		virtual void f_15() = 0;
		virtual void f_16() = 0;
		virtual void f_17() = 0;
		virtual void f_18() = 0;
		virtual void f_19() = 0;
		virtual void f_20() = 0;
		virtual void f_21() = 0;
		virtual void f_22() = 0;

		uint8_t GetCurrentState() const;
	};

	class aiTaskTree
	{
	public:
		aiTask* FindTaskByTypeActive(uint32_t taskType);
	};
}
