#pragma once

namespace rage
{
	class alignas(16) Vec3V
	{
	public:
		float X;
		float Y;
		float Z;
		float padding;
	};
	static_assert(sizeof(Vec3V) == 0x10);
}
