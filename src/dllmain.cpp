#include <Windows.h>
#include "LaserSight.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		spdlog::set_default_logger(spdlog::basic_logger_mt("file_logger", "LaserSightWeaponComponent.log"));
		spdlog::flush_every(std::chrono::seconds(30));
		LaserSight::InstallHooks();
	}
	else if (reason == DLL_PROCESS_DETACH)
	{
		spdlog::shutdown();
	}

    return TRUE;
}
