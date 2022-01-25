#include <Windows.h>
#include "Addresses.h"
#include "LaserBeam.h"
#include "LaserSight.h"
#include "ExtendedWeaponComponentLaserSightInfo.h"
#include "Replay.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <MinHook.h>
#include "Resources.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		spdlog::set_default_logger(spdlog::basic_logger_mt("file_logger", LASERSIGHTS_FILENAME ".log"));
		spdlog::flush_every(std::chrono::seconds(30));
		spdlog::set_level(spdlog::level::debug);

#define RETURN_IF_FAILED(success, message)                                        \
		if (!(success)) {                                                         \
			spdlog::error(message);                                               \
			spdlog::default_logger()->flush();                                    \
			MessageBoxA(NULL,                                                     \
				"An error occurred during initialization:\r\n"                    \
				" " message "\r\n\r\n"                                            \
				"Check your " LASERSIGHTS_FILENAME ".log file for more details.", \
				"Laser Sights - Initialization error",                            \
				MB_OK);                                                           \
			return TRUE;                                                          \
		}

		RETURN_IF_FAILED(Addresses.Init(),                                      "Failed to initialize AddressManager");
		RETURN_IF_FAILED(MH_Initialize() == MH_OK,                              "Failed to initialize MinHook");
		RETURN_IF_FAILED(ExtendedWeaponComponentLaserSightInfo::InstallHooks(), "Failed to install ExtendedWeaponComponentLaserSightInfo hooks");
		RETURN_IF_FAILED(LaserBeam::Init(hModule),                              "Failed to initialize LaserBeam module");
		RETURN_IF_FAILED(LaserSight::InstallHooks(),                            "Failed to install LaserSight hooks");
		RETURN_IF_FAILED(Replay::InstallHooks(),                                "Failed to install Replay hooks");
		RETURN_IF_FAILED(MH_EnableHook(MH_ALL_HOOKS) == MH_OK,                  "Failed to enable hooks");

#undef RETURN_IF_FAILED
	}
	else if (reason == DLL_PROCESS_DETACH)
	{
		spdlog::shutdown();
	}

    return TRUE;
}
