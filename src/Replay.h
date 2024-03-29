#pragma once
#include "Vector.h"

namespace rage { class fwEntity; }

class Replay
{
public:
	static bool InstallHooks();
	static void RecordLaserSightState(rage::fwEntity* weaponObject, bool isOn, const rage::Vec3V& dir);
};

