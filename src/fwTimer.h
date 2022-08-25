#pragma once
#include <cstdint>

namespace rage
{
	struct fwTimeSet
	{
		uint32_t TotalTimeMs;
		uint32_t NumUpdates;
		uint32_t LastUpdateTimeMs;
		float FrameTime;
		uint32_t FrameTimeMs;
		float FrameTimeReciprocal;
		float field_18;
	};

	struct fwTimer
	{
		static fwTimeSet& GameTime();
	};
}