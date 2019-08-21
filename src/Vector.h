#pragma once
#include <xmmintrin.h>

namespace rage
{
	struct alignas(16) Vec3V
	{
		union
		{
			struct
			{
				float x, y, z;
			};
			float f[4];
			__m128 v;
		};
	};
	static_assert(sizeof(Vec3V) == 0x10);

	struct alignas(16) Vec4V
	{
		union
		{
			struct
			{
				float x, y, z, w;
			};
			float f[4];
			__m128 v;
		};
	};
	static_assert(sizeof(Vec4V) == 0x10);
}
