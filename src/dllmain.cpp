#include <Windows.h>
#include "LaserBeam.h"
#include "LaserSight.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <MinHook.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		spdlog::set_default_logger(spdlog::basic_logger_mt("file_logger", "LaserSightWeaponComponent.log"));
		spdlog::flush_every(std::chrono::seconds(30));
		spdlog::set_level(spdlog::level::debug);
		MH_Initialize();
		LaserBeam::InstallHooks();
		LaserSight::InstallHooks();
		MH_EnableHook(MH_ALL_HOOKS);
	}
	else if (reason == DLL_PROCESS_DETACH)
	{
		spdlog::shutdown();
	}

    return TRUE;
}
