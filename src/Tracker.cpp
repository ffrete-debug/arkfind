#include "Tracker.h"

#include "Scanner.h"
#include "Text.h"

#include <vector>

namespace ArkFind
{
	void Say(AShooterPlayerController* player, const std::string& text)
	{
		Sdk::SendChat(player, PluginState::Get().Cfg().Message("Prefix", "[ArkFind] ") + text);
	}

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
		const bool visible = Sdk::FindDinoById(target.ActorId, current);
		if (visible)
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
			const std::string text = Text::Fill(
				config_.Message("Arrived", "You made it - {name} is right here."),
				{ {"name", target.DisplayName} });

			Say(player, text);
			Sdk::SendNotification(player, text, 6.0f);

			target.Active = false;
			target.Arrived = true;
			return;
		}

		// Standing next to the last known spot with nothing resolvable there means
		// the creature died or was destroyed, not that it streamed out. No point
		// steering the player around for the rest of the timeout.
		if (!visible && Geo::Distance3D(playerLocation, target.LastKnownLocation) <= options.ArrivalRadiusCm * 4.0)
		{
			Say(player, Text::Fill(
				config_.Message("TargetLost", "Lost {name} (it died or moved out of range). Tracking stopped."),
				{ {"name", target.DisplayName} }));
			target.Active = false;
			return;
		}

		// The dino unloaded and never came back before the timeout.
		if (config_.TrackingTimeoutSeconds > 0 && target.TicksSinceSeen >= config_.TrackingTimeoutSeconds)
		{
			Say(player, Text::Fill(
				config_.Message("TrackingTimedOut", "Tracking of {name} timed out after {seconds}s. Tracking stopped."),
				{ {"name", target.DisplayName}, {"seconds", std::to_string(config_.TrackingTimeoutSeconds)} }));
			target.Active = false;
			return;
		}

		Sdk::SendNotification(player,
			FormatDirection(target, playerLocation, yaw, options),
			static_cast<float>(config_.UpdateIntervalSeconds) + 0.5f);

		(void)playerId;
	}
}
