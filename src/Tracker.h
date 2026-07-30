#pragma once

#include <string>

#include "Config.h"
#include "Session.h"

// Declared as a struct to match the SDK; see the note in Scanner.h.
struct AShooterPlayerController;

namespace ArkFind
{
	// Single owner of the plugin's mutable state: the loaded config plus one
	// session per player. The chat commands mutate it, the one-second tick reads
	// it and pushes direction updates.
	class PluginState
	{
	public:
		static PluginState& Get();

		const Config& Cfg() const { return config_; }

		// Reads config.json. Never leaves the config in a half-applied state: a
		// failed load keeps whatever was already active.
		bool ReloadConfig(std::string& error);

		PlayerSession& SessionFor(uint64_t playerId);
		void ForgetPlayer(uint64_t playerId);

		// Called once per second by the ArkApi timer. Rate limits itself down to
		// Config::UpdateIntervalSeconds.
		void Tick();

	private:
		PluginState() = default;

		void UpdateTracking(uint64_t playerId, PlayerSession& session, AShooterPlayerController* player);

		Config config_;
		SessionStore sessions_;
		int secondsSinceUpdate_ = 0;
	};

	// Sends one chat line to a player, prefixed with Messages.Prefix.
	void Say(AShooterPlayerController* player, const std::string& text);

	namespace Commands
	{
		void Register();
		void Unregister();
	}
}
