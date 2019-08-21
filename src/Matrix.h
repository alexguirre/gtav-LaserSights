#pragma once
#include "Vector.h"

namespace rage
{
	struct alignas(16) Mat34V
	{
		union
		{
			float m[4][4];
			Vec4V r[4];
		};
	};
	static_assert(sizeof(Mat34V) == 0x40);
}
