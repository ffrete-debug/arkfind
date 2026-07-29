#include "Scanner.h"
#include "Tracker.h"

#include <API/ARK/Ark.h>

#include <string>
#include <utility>
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
				return ArkApi::Tools::Utf8Encode(*value);
			}

			// Everything after the command word, trimmed.
			std::string Argument(const FString* message)
			{
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

			bool Allowed(AShooterPlayerController* player)
			{
				const PluginState& state = PluginState::Get();
				if (!state.Cfg().RequireAdmin || Sdk::IsAdmin(player))
				{
					return true;
				}

				Sdk::SendChat(player, state.Cfg().Message("NoPermission",
					"You do not have permission to use ArkFind."));
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
				Sdk::SendChat(player, cfg.Message("Tracking", "Tracking " + hit.Dino.DisplayName
					+ " (lvl " + std::to_string(hit.Dino.Level) + "). Follow the arrow. /findstop to cancel."));

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
				const uint64_t playerId = Sdk::GetPlayerId(player);
				PlayerSession& session = state.SessionFor(playerId);

				const std::string query = Argument(message);
				const Vec3 origin = Sdk::GetPlayerLocation(player);

				const SearchOptions options = cfg.ToSearchOptions();
				const std::vector<DinoInfo> scanned = Sdk::ScanDinos(origin, options.RadiusCm);

				session.LastQuery = query;
				session.LastResults = RankResults(scanned, origin, query, options);

				if (session.LastResults.empty())
				{
					Sdk::SendChat(player, cfg.Message("NoResults",
						"Nothing matching \"" + query + "\" within "
						+ std::to_string(static_cast<int>(cfg.SearchRadiusMeters)) + "m."));
					return;
				}

				// A single unambiguous hit needs no picking step.
				if (session.LastResults.size() == 1 && !query.empty())
				{
					StartTracking(player, session, session.LastResults[0]);
					return;
				}

				Sdk::SendChat(player, cfg.Message("ResultsHeader",
					"Found " + std::to_string(session.LastResults.size())
					+ " - pick one with /findpick <number>:"));

				for (size_t i = 0; i < session.LastResults.size(); ++i)
				{
					Sdk::SendChat(player, FormatHitLine(static_cast<int>(i) + 1, session.LastResults[i]));
				}
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
					Sdk::SendChat(player, cfg.Message("NothingToPick",
						"Run /finddino <name> first."));
					return;
				}

				const int index = ParseSelection(Argument(message), session.LastResults.size());
				if (index < 0)
				{
					Sdk::SendChat(player, cfg.Message("BadSelection",
						"Pick a number between 1 and " + std::to_string(session.LastResults.size()) + "."));
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
				PlayerSession& session = state.SessionFor(Sdk::GetPlayerId(player));
				session.Target = TrackedTarget{};

				Sdk::SendChat(player, state.Cfg().Message("Stopped", "ArkFind tracking stopped."));
			}

			// A quick census of what is around, so players know what to search for.
			void FindHere(AShooterPlayerController* player, FString* /*message*/, EChatSendMode::Type /*mode*/)
			{
				if (player == nullptr || !Allowed(player))
				{
					return;
				}

				const Config& cfg = PluginState::Get().Cfg();
				const SearchOptions options = cfg.ToSearchOptions();
				const Vec3 origin = Sdk::GetPlayerLocation(player);

				SearchOptions census = options;
				census.MaxResults = 0;
				const std::vector<SearchHit> hits = RankResults(Sdk::ScanDinos(origin, options.RadiusCm),
					origin, "", census);

				if (hits.empty())
				{
					Sdk::SendChat(player, cfg.Message("NoResults", "No creatures loaded near you."));
					return;
				}

				// Species -> count, keeping the nearest example per species.
				std::vector<std::pair<std::string, int>> species;
				for (const SearchHit& hit : hits)
				{
					bool merged = false;
					for (auto& entry : species)
					{
						if (entry.first == hit.Dino.DisplayName)
						{
							++entry.second;
							merged = true;
							break;
						}
					}
					if (!merged)
					{
						species.emplace_back(hit.Dino.DisplayName, 1);
					}
				}

				Sdk::SendChat(player, std::to_string(hits.size()) + " creatures, "
					+ std::to_string(species.size()) + " species within "
					+ std::to_string(static_cast<int>(cfg.SearchRadiusMeters)) + "m:");

				const size_t limit = species.size() < 20 ? species.size() : 20;
				for (size_t i = 0; i < limit; ++i)
				{
					Sdk::SendChat(player, "- " + species[i].first + " x" + std::to_string(species[i].second));
				}
			}

			void FindCfg(AShooterPlayerController* player, FString* message, EChatSendMode::Type /*mode*/)
			{
				if (player == nullptr)
				{
					return;
				}

				if (!Sdk::IsAdmin(player))
				{
					Sdk::SendChat(player, "Admins only.");
					return;
				}

				if (Argument(message) != "reload")
				{
					Sdk::SendChat(player, "Usage: /findcfg reload");
					return;
				}

				std::string error;
				if (PluginState::Get().ReloadConfig(error))
				{
					Sdk::SendChat(player, "ArkFind config reloaded.");
				}
				else
				{
					Sdk::SendChat(player, "ArkFind config reload failed: " + error);
				}
			}
		}

		void Register()
		{
			ArkApi::GetCommands()->AddChatCommand(CmdFind, &FindDino);
			ArkApi::GetCommands()->AddChatCommand(CmdPick, &FindPick);
			ArkApi::GetCommands()->AddChatCommand(CmdStop, &FindStop);
			ArkApi::GetCommands()->AddChatCommand(CmdHere, &FindHere);
			ArkApi::GetCommands()->AddChatCommand(CmdCfg, &FindCfg);
		}

		void Unregister()
		{
			ArkApi::GetCommands()->RemoveChatCommand(CmdFind);
			ArkApi::GetCommands()->RemoveChatCommand(CmdPick);
			ArkApi::GetCommands()->RemoveChatCommand(CmdStop);
			ArkApi::GetCommands()->RemoveChatCommand(CmdHere);
			ArkApi::GetCommands()->RemoveChatCommand(CmdCfg);
		}
	}
}
