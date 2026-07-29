#include "Tracker.h"

#include "Scanner.h"

#include <vector>

namespace ArkFind
{
	PluginState& PluginState::Get()
	{
		static PluginState instance;
		return instance;
	}

	bool PluginState::ReloadConfig(std::string& error)
	{
		Config loaded;
		if (!LoadConfig(Sdk::PluginConfigPath(), loaded, error))
		{
			return false;
		}

		config_ = loaded;
		return true;
	}

	PlayerSession& PluginState::SessionFor(uint64_t playerId)
	{
		return sessions_[playerId];
	}

	void PluginState::ForgetPlayer(uint64_t playerId)
	{
		sessions_.erase(playerId);
	}

	void PluginState::Tick()
	{
		if (++secondsSinceUpdate_ < config_.UpdateIntervalSeconds)
		{
			return;
		}
		secondsSinceUpdate_ = 0;

		// Collect first: UpdateTracking can erase sessions, which would invalidate
		// an iterator held across the loop.
		std::vector<uint64_t> active;
		active.reserve(sessions_.size());
		for (const auto& entry : sessions_)
		{
			if (entry.second.Target.Active)
			{
				active.push_back(entry.first);
			}
		}

		for (const uint64_t playerId : active)
		{
			const auto it = sessions_.find(playerId);
			if (it == sessions_.end())
			{
				continue;
			}

			AShooterPlayerController* player = Sdk::FindPlayer(playerId);
			if (player == nullptr)
			{
				// Disconnected mid-hunt; drop the session so it does not leak.
				sessions_.erase(it);
				continue;
			}

			UpdateTracking(playerId, it->second, player);
		}
	}

	void PluginState::UpdateTracking(uint64_t playerId, PlayerSession& session, AShooterPlayerController* player)
	{
		TrackedTarget& target = session.Target;

		DinoInfo current;
		if (Sdk::FindDinoById(target.ActorId, current))
		{
			target.LastKnownLocation = current.Location;
			target.DisplayName = current.DisplayName;
			target.TicksSinceSeen = 0;
		}
		else
		{
			// The dino is unloaded or dead. Keep guiding to the last known spot for
			// a while, since the player is usually still far away.
			target.TicksSinceSeen += config_.UpdateIntervalSeconds;
		}

		const Vec3 playerLocation = Sdk::GetPlayerLocation(player);
		const double yaw = Sdk::GetPlayerYaw(player);
		const DirectionOptions options = config_.ToDirectionOptions();

		if (HasArrived(target, playerLocation, options.ArrivalRadiusCm))
		{
			const std::string message = config_.Message("Arrived", "FOUND {name} - you are on top of it.");
			std::string text = message;
			const size_t slot = text.find("{name}");
			if (slot != std::string::npos)
			{
				text.replace(slot, 6, target.DisplayName);
			}

			Sdk::SendChat(player, text);
			Sdk::SendNotification(player, text, 6.0f);

			target.Active = false;
			target.Arrived = true;
			return;
		}

		if (config_.TrackingTimeoutSeconds > 0 && target.TicksSinceSeen >= config_.TrackingTimeoutSeconds)
		{
			Sdk::SendChat(player, config_.Message("LostTarget",
				"Lost track of " + target.DisplayName + " - it despawned or moved out of range. Run /finddino again."));
			target.Active = false;
			return;
		}

		Sdk::SendNotification(player,
			FormatDirection(target, playerLocation, yaw, options),
			static_cast<float>(config_.UpdateIntervalSeconds) + 0.5f);

		(void)playerId;
	}
}
