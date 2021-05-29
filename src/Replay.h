#pragma once

namespace rage { class fwEntity; }

class Replay
{
public:
	static void InstallHooks();
	static void RecordLaserSightState(rage::fwEntity* weaponObject, bool isOn);
};

