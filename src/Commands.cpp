#include "Scanner.h"
#include "Text.h"
#include "Tracker.h"

#include <API/ARK/Ark.h>
#include <Logger/Logger.h>

#include <string>
#include <vector>

namespace ArkFind
{
	namespace Commands
	{
		namespace
		{
			const FString CmdFind = FString("/finddino");
			const FString CmdPick = FString("/findpick");
			const FString CmdStop = FString("/findstop");
			const FString CmdHere = FString("/findhere");
			const FString CmdCfg = FString("/findcfg");

			std::string ToStd(const FString& value)
			{
				return ArkApi::Tools::Utf8Encode(std::wstring(*value));
			}

			// Everything after the command word, trimmed.
			std::string Argument(const FString* message)
			{
				if (message == nullptr)
				{
					return "";
				}

				std::string raw = ToStd(*message);

				const size_t space = raw.find(' ');
				if (space == std::string::npos)
				{
					return "";
				}

				raw = raw.substr(space + 1);

				const size_t first = raw.find_first_not_of(" \t");
				if (first == std::string::npos)
				{
					return "";
				}
				const size_t last = raw.find_last_not_of(" \t");
				return raw.substr(first, last - first + 1);
			}

			std::string RadiusText(const Config& cfg)
			{
				return Text::Number(cfg.SearchRadiusMeters, 0);
			}

			bool Allowed(AShooterPlayerController* player)
			{
				const Config& cfg = PluginState::Get().Cfg();
				if (!cfg.RequireAdmin || Sdk::IsAdmin(player))
				{
					return true;
				}

				Say(player, cfg.Message("NoPermission", "You are not allowed to use this command."));
				return false;
			}

			void StartTracking(AShooterPlayerController* player, PlayerSession& session, const SearchHit& hit)
			{
				TrackedTarget& target = session.Target;
				target.Active = true;
				target.Arrived = false;
				target.ActorId = hit.Dino.ActorId;
				target.DisplayName = hit.Dino.DisplayName;
				target.BlueprintPath = hit.Dino.BlueprintPath;
				target.LastKnownLocation = hit.Dino.Location;
				target.TicksSinceSeen = 0;

				const Config& cfg = PluginState::Get().Cfg();
				Say(player, Text::Fill(
					cfg.Message("TrackingStarted", "Now tracking {name}. Walk and I will keep calling the turns."),
					{ {"name", hit.Dino.DisplayName} }));

				// Give an immediate first bearing instead of waiting for the tick.
				Sdk::SendNotification(player,
					FormatDirection(target, Sdk::GetPlayerLocation(player), Sdk::GetPlayerYaw(player),
						cfg.ToDirectionOptions()),
					4.0f);
			}

			void FindDino(AShooterPlayerController* player, FString* message, EChatSendMode::Type /*mode*/)
			{
				if (player == nullptr || !Allowed(player))
				{
					return;
				}

				PluginState& state = PluginState::Get();
				const Config& cfg = state.Cfg();
				PlayerSession& session = state.SessionFor(Sdk::GetPlayerId(player));

				const std::string query = Argument(message);
				const Text::Placeholders context = { {"query", query}, {"radius", RadiusText(cfg)} };

				Say(player, Text::Fill(cfg.Message("Searching", "Searching for '{query}' within {radius}m..."), context));

				const Vec3 origin = Sdk::GetPlayerLocation(player);
				const SearchOptions options = cfg.ToSearchOptions();

				session.LastQuery = query;
				session.LastResults = RankResults(Sdk::ScanDinos(origin, options.RadiusCm), origin, query, options);

				if (session.LastResults.empty())
				{
					Say(player, Text::Fill(
						cfg.Message("NoResults", "Nothing matching '{query}' found within {radius}m."), context));
					return;
				}

				const std::string count = std::to_string(session.LastResults.size());

				Say(player, Text::Fill(
					cfg.Message("ResultsHeader", "Found {count} match(es) for '{query}'. Use /findpick <n> to track one:"),
					{ {"count", count}, {"query", query}, {"radius", RadiusText(cfg)} }));

				for (size_t i = 0; i < session.LastResults.size(); ++i)
				{
					Say(player, FormatHitLine(static_cast<int>(i) + 1, session.LastResults[i]));
				}

				Say(player, cfg.Message("ResultsFooter",
					"Type /findpick <n> to start directions, /findstop to cancel."));
			}

			void FindPick(AShooterPlayerController* player, FString* message, EChatSendMode::Type /*mode*/)
			{
				if (player == nullptr || !Allowed(player))
				{
					return;
				}

				PluginState& state = PluginState::Get();
				const Config& cfg = state.Cfg();
				PlayerSession& session = state.SessionFor(Sdk::GetPlayerId(player));

				if (session.LastResults.empty())
				{
					Say(player, cfg.Message("NoSearchYet", "Run /finddino first, then /findpick <n>."));
					return;
				}

				const std::string argument = Argument(message);
				const int index = ParseSelection(argument, session.LastResults.size());
				if (index < 0)
				{
					Say(player, Text::Fill(
						cfg.Message("InvalidSelection", "'{argument}' is not a valid pick. Choose 1-{count}."),
						{ {"argument", argument}, {"count", std::to_string(session.LastResults.size())} }));
					return;
				}

				StartTracking(player, session, session.LastResults[static_cast<size_t>(index)]);
			}

			void FindStop(AShooterPlayerController* player, FString* /*message*/, EChatSendMode::Type /*mode*/)
			{
				if (player == nullptr)
				{
					return;
				}

				PluginState& state = PluginState::Get();
				const Config& cfg = state.Cfg();
				PlayerSession& session = state.SessionFor(Sdk::GetPlayerId(player));

				if (!session.Target.Active)
				{
					Say(player, cfg.Message("NotTracking", "You are not tracking anything right now."));
					return;
				}

				session.Target = TrackedTarget{};
				Say(player, cfg.Message("TrackingStopped", "Stopped tracking."));
			}

			// Prints the player's own position, which is also how you calibrate the
			// Map.* GPS values on a custom map.
			void FindHere(AShooterPlayerController* player, FString* /*message*/, EChatSendMode::Type /*mode*/)
			{
				if (player == nullptr || !Allowed(player))
				{
					return;
				}

				const Config& cfg = PluginState::Get().Cfg();
				const Vec3 location = Sdk::GetPlayerLocation(player);
				const Geo::MapCoords coords = Geo::ToMapCoords(location, cfg.Map);

				Say(player, Text::Fill(
					cfg.Message("HereLocation", "You are at lat {lat} lon {lon} (x {x} y {y} z {z})."),
					{
						{"lat", Text::Number(coords.Lat, 1)},
						{"lon", Text::Number(coords.Lon, 1)},
						{"x", Text::Number(location.X, 0)},
						{"y", Text::Number(location.Y, 0)},
						{"z", Text::Number(location.Z, 0)},
					}));
			}

			void FindCfg(AShooterPlayerController* player, FString* message, EChatSendMode::Type /*mode*/)
			{
				if (player == nullptr)
				{
					return;
				}

				const Config& cfg = PluginState::Get().Cfg();
				if (cfg.RequireAdmin && !Sdk::IsAdmin(player))
				{
					Say(player, cfg.Message("NoPermission", "You are not allowed to use this command."));
					return;
				}

				if (Argument(message) != "reload")
				{
					Say(player, "Usage: /findcfg reload");
					return;
				}

				std::string error;
				if (PluginState::Get().ReloadConfig(error))
				{
					if (!error.empty())
					{
						Log::GetLog()->warn("ArkFind config loaded with corrections: {}", error);
					}
					Say(player, PluginState::Get().Cfg().Message("ConfigReloaded", "Configuration reloaded."));
				}
				else
				{
					Log::GetLog()->warn("ArkFind config reload failed: {}", error);
					Say(player, cfg.Message("ConfigReloadFailed",
						"Could not reload configuration - see the server log."));
				}
			}
		}

		void Register()
		{
			ArkApi::GetCommands().AddChatCommand(CmdFind, &FindDino);
			ArkApi::GetCommands().AddChatCommand(CmdPick, &FindPick);
			ArkApi::GetCommands().AddChatCommand(CmdStop, &FindStop);
			ArkApi::GetCommands().AddChatCommand(CmdHere, &FindHere);
			ArkApi::GetCommands().AddChatCommand(CmdCfg, &FindCfg);
		}

		void Unregister()
		{
			ArkApi::GetCommands().RemoveChatCommand(CmdFind);
			ArkApi::GetCommands().RemoveChatCommand(CmdPick);
			ArkApi::GetCommands().RemoveChatCommand(CmdStop);
			ArkApi::GetCommands().RemoveChatCommand(CmdHere);
			ArkApi::GetCommands().RemoveChatCommand(CmdCfg);
		}
	}
}
