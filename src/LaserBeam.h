#pragma once
#include "Vector.h"
#include <Windows.h>

class LaserBeam
{
public:
	static bool Init(HMODULE hModule);
	static void DrawBeam(float width, const rage::Vec3V& from, const rage::Vec3V& to, const rage::Vec3V& rightVector, const rage::Vec4V& colorXYZ_beamIntensityW);
};

