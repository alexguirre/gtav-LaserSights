#pragma once
#include "Vector.h"

class LaserBeam
{
public:
	static void InstallHooks();
	static void DrawBeam(float width, const rage::Vec3V& from, const rage::Vec3V& to, const rage::Vec3V& rightVector, const rage::Vec4V& color);
	static void DrawDot(const rage::Vec3V& position, const rage::Vec3V& normal, const rage::Vec4V& color);
};

