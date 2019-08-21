#pragma once
#include <stdint.h>
#include "Vector.h"

class CCoronas
{
public:
	void Draw(const rage::Vec3V& position, float size, uint32_t color, float intensity, float zbias, const rage::Vec3V& direction, float a8, float innerAngle, float outerAngle, uint16_t flags);

	static CCoronas* Instance();
};
