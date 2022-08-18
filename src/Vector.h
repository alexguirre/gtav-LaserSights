#pragma once
#include <DirectXMath.h>
#include <xmmintrin.h>

namespace rage
{
	inline __m128 dot(__m128 a, __m128 b)
	{
		// aX * bX + aY * bY + aZ * bZ
		__m128 mul = _mm_mul_ps(a, b);                                  // mul = (aX*bX, aY*bY, aZ*bZ, _) = (dx, dy, dz, _)
		__m128 yz = _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(0, 0, 2, 1));  // yz = (dy, dz, _, _)
		__m128 addXY = _mm_add_ps(mul, yz);                             // addXY = (dx, dy, dz, _) + (dy, _, _, _) = (dx+dy, _, _, _)
		__m128 z = _mm_shuffle_ps(yz, yz, _MM_SHUFFLE(0, 0, 0, 1));     // z = (dz, _, _, _)
		__m128 addXYZ = _mm_add_ss(addXY, z);                           // addXYZ = (dx+dy, _, _, _) + (dz, _, _, _) = (dx+dy+dz, _, _, _)
		return _mm_shuffle_ps(addXYZ, addXYZ, _MM_SHUFFLE(0, 0, 0, 0)); // return (dx+dy+dz, dx+dy+dz, dx+dy+dz, dx+dy+dz)
	}

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
			: v{ _mm_setzero_ps() }
		{}

		inline Vec3V(float x, float y, float z)
			: v{ _mm_set_ps(0.0f, z, y, x) }
		{}

		inline Vec3V(__m128 v)
			: v{ v }
		{}

		inline Vec3V(const Vec3V& v)
			: v{ v.v }
		{}

		inline Vec3V& operator=(const Vec3V& other)
		{
			v = other.v;

			return *this;
		}

		inline float Length() const { return _mm_cvtss_f32(_mm_sqrt_ss(dot(v, v))); }
		inline float LengthSquared() const { return _mm_cvtss_f32(dot(v, v)); }
		inline Vec3V Normalized() const { return _mm_mul_ps(v, _mm_rsqrt_ps(dot(v, v))); }
		
		inline Vec3V Cross(const Vec3V& b) const
		{
			// t = (aY, aZ, aX) * (bZ, bX, bY) // element wise multiplication
			// u = (aZ, aX, aY) * (bY, bZ, bX)
			// r = t - u

			__m128 t = _mm_mul_ps(_mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(b.v, b.v, _MM_SHUFFLE(3, 1, 0, 2)));
			__m128 u = _mm_mul_ps(_mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 1, 0, 2)), _mm_shuffle_ps(b.v, b.v, _MM_SHUFFLE(3, 0, 2, 1)));
			return _mm_sub_ps(t, u);
		}

		inline float Dot(const Vec3V& b) const { return _mm_cvtss_f32(dot(v, b.v)); }

		inline void Normalize() { v = this->Normalized().v; }

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
			: v{ _mm_setzero_ps() }
		{}

		inline Vec4V(float x, float y, float z, float w)
			: v{ _mm_set_ps(w, z, y, x) }
		{}

		inline Vec4V(__m128 v)
			: v{ v }
		{}

		inline Vec4V(const Vec4V& v)
			: v{ v.v }
		{}

		inline Vec4V& operator=(const Vec4V& other)
		{
			v = other.v;

			return *this;
		}

		inline float Length() const { return _mm_cvtss_f32(_mm_sqrt_ss(dot(v, v))); }
		inline float LengthSquared() const { return _mm_cvtss_f32(dot(v, v)); }
		inline Vec4V Normalized() const { return _mm_mul_ps(v, _mm_rsqrt_ps(dot(v, v))); }

		inline void Normalize() { v = this->Normalized().v; }

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
