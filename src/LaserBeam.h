#pragma once
#include "Vector.h"

class LaserBeam
{
public:
	static bool InstallHooks();
	static void DrawBeam(float width, const rage::Vec3V& from, const rage::Vec3V& to, const rage::Vec3V& rightVector, const rage::Vec3V& color);
};

