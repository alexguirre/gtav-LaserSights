#pragma once
#include "Vector.h"

namespace rage
{
	struct alignas(16) Mat34V
	{
		union
		{
			float m[4][4];
			Vec3V r[4];
		};

		inline Mat34V()
		{
			// empty
		}

		inline Mat34V(const Vec3V& r0, const Vec3V& r1, const Vec3V& r2, const Vec3V& r3)
		{
			r[0] = r0;
			r[1] = r1;
			r[2] = r2;
			r[3] = r3;
		}

		inline Mat34V(__m128 r0, __m128 r1, __m128 r2, __m128 r3)
		{
			r[0] = r0;
			r[1] = r1;
			r[2] = r2;
			r[3] = r3;
		}

		inline Mat34V(const Mat34V& v)
		{
			r[0] = v.r[0];
			r[1] = v.r[1];
			r[2] = v.r[2];
			r[3] = v.r[3];
		}

		inline Mat34V& operator=(const Mat34V& other)
		{
			r[0] = other.r[0];
			r[1] = other.r[1];
			r[2] = other.r[2];
			r[3] = other.r[3];

			return *this;
		}

		inline Vec3V& Forward() { return r[0]; }
		inline const Vec3V& Forward() const { return r[0]; }
		inline Vec3V& Right() { return r[1]; }
		inline const Vec3V& Right() const { return r[1]; }
		inline Vec3V& Up() { return r[2]; }
		inline const Vec3V& Up() const { return r[2]; }
		inline Vec3V& Position() { return r[3]; }
		inline const Vec3V& Position() const { return r[3]; }

		static const Mat34V& Identity()
		{
			static Mat34V identity(
				_mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f),
				_mm_set_ps(0.0f, 0.0f, 1.0f, 0.0f),
				_mm_set_ps(0.0f, 1.0f, 0.0f, 0.0f),
				_mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f)
			);
		
			return identity;
		}
	};
	static_assert(sizeof(Mat34V) == 0x40);
}
