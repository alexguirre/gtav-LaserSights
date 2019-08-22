#pragma once
#include "Vector.h"

class LaserBeam
{
public:
	static void InstallHooks();
	static void DrawBeam(const rage::Vec3V& from, const rage::Vec3V& to, const rage::Vec3V& rightVector);
	// TODO: DrawDot
};

