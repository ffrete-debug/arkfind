// ArkFind - find any creature, vanilla or modded, and get guided to it.
//
// Entry points for the ServerAPI plugin loader.

#include "Tracker.h"

#include <API/ARK/Ark.h>
#include <Logger/Logger.h>

#include <string>

namespace
{
	const FString TimerId = FString("ArkFind.Tick");

	void OnTimer()
	{
		ArkFind::PluginState::Get().Tick();
	}

	void Load()
	{
		Log::Get().Init("ArkFind");

		std::string error;
		if (!ArkFind::PluginState::Get().ReloadConfig(error))
		{
			// Defaults are usable, so a missing config is a warning, not a failure.
			Log::GetLog()->warn("ArkFind config not loaded ({}); using defaults", error);
		}
		else if (!error.empty())
		{
			Log::GetLog()->warn("ArkFind config loaded with corrections: {}", error);
		}

		ArkFind::Commands::Register();
		ArkApi::GetCommands().AddOnTimerCallback(TimerId, &OnTimer);

		Log::GetLog()->info("ArkFind loaded");
	}

	void Unload()
	{
		ArkApi::GetCommands().RemoveOnTimerCallback(TimerId);
		ArkFind::Commands::Unregister();

		Log::GetLog()->info("ArkFind unloaded");
	}
}

BOOL APIENTRY DllMain(HMODULE /*module*/, DWORD reason, LPVOID /*reserved*/)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		Load();
		break;
	case DLL_PROCESS_DETACH:
		Unload();
		break;
	default:
		break;
	}

	return TRUE;
}
