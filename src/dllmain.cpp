#include <Windows.h>
#include "Addresses.h"
#include "LaserBeam.h"
#include "LaserSight.h"
#include "ExtendedWeaponComponentLaserSightInfo.h"
#include "Replay.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/details/null_mutex.h>
#include <MinHook.h>
#include "Resources.h"

#ifdef USE_FIVEM_CONSOLE
// Print to FiveM's game console
static void FxPrint(const std::string& channel, const std::string& message) // this requires _ITERATOR_DEBUG_LEVEL=0, otherwise std::string layout won't match conhost-v2.dll
{
	using TFunc = decltype(&FxPrint);

	static TFunc func = (TFunc)GetProcAddress(GetModuleHandle("conhost-v2.dll"), "?Print@ConHost@@YAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@0@Z");

	func ? func(channel, message) : (void)0;
}

class FxSink : public spdlog::sinks::base_sink<spdlog::details::null_mutex>
{
protected:
	void sink_it_(const spdlog::details::log_msg& msg) override
    {
		spdlog::memory_buf_t formatted;
		formatter_->format(msg, formatted);
		FxPrint("lasersights", formatted);
    }

	void flush_() override {}
};
#endif

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		auto logger = std::make_shared<spdlog::logger>("main", spdlog::sinks_init_list(
		{
			std::make_shared<spdlog::sinks::basic_file_sink_mt>(LASERSIGHTS_FILENAME ".log"),
#ifdef USE_FIVEM_CONSOLE
			std::make_shared<FxSink>(),
#endif
		}));
		logger->set_pattern("[%Y-%m-%d %T.%e] [%l] %v");
		logger->flush_on(spdlog::level::err);
		spdlog::set_default_logger(logger);
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
			return FALSE;                                                         \
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
