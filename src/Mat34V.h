#pragma once

namespace rage
{
	class alignas(16) Mat34V
	{
	public:
		float M[4][4];
	};
	static_assert(sizeof(Mat34V) == 0x40);
}
