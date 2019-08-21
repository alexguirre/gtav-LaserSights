#pragma once
#include <xmmintrin.h>
#include <DirectXMath.h>

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

		inline Vec3V()
			: v(_mm_setzero_ps())
		{}

		inline Vec3V(float x, float y, float z)
			: v(_mm_set_ps(0.0f, z, y, x))
		{}

		inline Vec3V(__m128 v)
			: v(v)
		{}

		inline Vec3V(const Vec3V& v)
			: v(v.v)
		{}

		inline Vec3V& operator=(const Vec3V& other)
		{
			v = other.v;

			return *this;
		}

		inline float Length() const { return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(v, v, 0b01110001))); }
		inline float LengthSquared() const { return _mm_cvtss_f32(_mm_dp_ps(v, v, 0b01110001)); }
		inline Vec3V Normalized() const { return _mm_mul_ps(v, _mm_rsqrt_ps(_mm_dp_ps(v, v, 0b01111111))); }

		inline Vec3V operator+(const Vec3V& other) const { return _mm_add_ps(v, other.v); }
		inline Vec3V operator-(const Vec3V& other) const { return _mm_sub_ps(v, other.v); }
		inline Vec3V operator*(const Vec3V& other) const { return _mm_mul_ps(v, other.v); }
		inline Vec3V operator/(const Vec3V& other) const { return _mm_div_ps(v, other.v); }
		inline Vec3V& operator+=(const Vec3V& other) { v = _mm_add_ps(v, other.v); return *this; }
		inline Vec3V& operator-=(const Vec3V& other) { v = _mm_sub_ps(v, other.v); return *this; }
		inline Vec3V& operator*=(const Vec3V& other) { v = _mm_mul_ps(v, other.v); return *this; }
		inline Vec3V& operator/=(const Vec3V& other) { v = _mm_div_ps(v, other.v); return *this; }

		inline Vec3V operator*(float value) const { return _mm_mul_ps(v, _mm_set1_ps(value)); }
		inline Vec3V operator/(float value) const { return _mm_div_ps(v, _mm_set1_ps(value)); }
		inline Vec3V& operator*=(float value) { v = _mm_mul_ps(v, _mm_set1_ps(value)); return *this; }
		inline Vec3V& operator/=(float value) { v = _mm_div_ps(v, _mm_set1_ps(value)); return *this; }
	};
	static_assert(sizeof(Vec3V) == 0x10);

	inline Vec3V operator*(float a, const Vec3V& b) { return b * a; }

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

		inline Vec4V()
			: v(_mm_setzero_ps())
		{}

		inline Vec4V(float x, float y, float z, float w)
			: v(_mm_set_ps(w, z, y, x))
		{}

		inline Vec4V(__m128 v)
			: v(v)
		{}

		inline Vec4V(const Vec4V& v)
			: v(v.v)
		{}

		inline Vec4V& operator=(const Vec4V& other)
		{
			v = other.v;

			return *this;
		}

		inline float Length() const { return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(v, v, 0b01110001))); }
		inline float LengthSquared() const { return _mm_cvtss_f32(_mm_dp_ps(v, v, 0b01110001)); }
		inline Vec4V Normalized() const { return _mm_mul_ps(v, _mm_rsqrt_ps(_mm_dp_ps(v, v, 0b01111111))); }

		inline Vec4V operator+(const Vec4V& other) const { return _mm_add_ps(v, other.v); }
		inline Vec4V operator-(const Vec4V& other) const { return _mm_sub_ps(v, other.v); }
		inline Vec4V operator*(const Vec4V& other) const { return _mm_mul_ps(v, other.v); }
		inline Vec4V operator/(const Vec4V& other) const { return _mm_div_ps(v, other.v); }
		inline Vec4V& operator+=(const Vec4V& other) { v = _mm_add_ps(v, other.v); return *this; }
		inline Vec4V& operator-=(const Vec4V& other) { v = _mm_sub_ps(v, other.v); return *this; }
		inline Vec4V& operator*=(const Vec4V& other) { v = _mm_mul_ps(v, other.v); return *this; }
		inline Vec4V& operator/=(const Vec4V& other) { v = _mm_div_ps(v, other.v); return *this; }

		inline Vec4V operator*(float value) const { return _mm_mul_ps(v, _mm_set1_ps(value)); }
		inline Vec4V operator/(float value) const { return _mm_div_ps(v, _mm_set1_ps(value)); }
		inline Vec4V& operator*=(float value) { v = _mm_mul_ps(v, _mm_set1_ps(value)); return *this; }
		inline Vec4V& operator/=(float value) { v = _mm_div_ps(v, _mm_set1_ps(value)); return *this; }
	};
	static_assert(sizeof(Vec4V) == 0x10);

	inline Vec4V operator*(float a, const Vec4V& b) { return b * a; }
}
