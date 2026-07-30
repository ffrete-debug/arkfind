#include "Scanner.h"

#include "Geo.h"
#include "NameMatch.h"

#include <API/ARK/Ark.h>

#include <cmath>

namespace ArkFind
{
	namespace Sdk
	{
		namespace
		{
			std::string ToStd(const FString& value)
			{
				return ArkApi::Tools::Utf8Encode(std::wstring(*value));
			}

			FString ToFString(const std::string& value)
			{
				return FString(ArkApi::Tools::Utf8Decode(value).c_str());
			}

			Vec3 ToVec3(const FVector& v)
			{
				Vec3 out;
				out.X = static_cast<double>(v.X);
				out.Y = static_cast<double>(v.Y);
				out.Z = static_cast<double>(v.Z);
				return out;
			}

			// ARK splits the persistent dino id across two 32 bit fields.
			uint64_t DinoId(APrimalDinoCharacter* dino)
			{
				const uint64_t high = static_cast<uint64_t>(dino->DinoID1Field());
				const uint64_t low = static_cast<uint64_t>(dino->DinoID2Field());
				return (high << 32) | low;
			}

			// Team ids below this are wild spawns; player tribes start well above it.
			constexpr int TamedTeamThreshold = 10000;

			bool FillDinoInfo(APrimalDinoCharacter* dino, DinoInfo& out)
			{
				if (dino == nullptr || dino->IsDead())
				{
					return false;
				}

				USceneComponent* root = dino->RootComponentField();
				if (root == nullptr)
				{
					return false;
				}

				out.Location = ToVec3(root->RelativeLocationField());
				out.IsAlive = true;
				out.ActorId = DinoId(dino);
				out.IsTamed = dino->TargetingTeamField() > TamedTeamThreshold;
				out.IsFemale = dino->bIsFemale()();
				if (UPrimalCharacterStatusComponent* status = dino->MyCharacterStatusComponentField())
				{
					out.Level = static_cast<int>(status->BaseCharacterLevelField())
						+ static_cast<int>(status->ExtraCharacterLevelField());
				}

				// The creature's own descriptive name is what makes mod dinos work
				// without a hardcoded species table.
				FString descriptive;
				dino->GetDescriptiveName(&descriptive);
				out.DisplayName = ToStd(descriptive);

				out.BlueprintPath = ToStd(ArkApi::GetApiUtils().GetBlueprint(dino));
				out.ClassName = NameMatch::ClassNameFromBlueprint(out.BlueprintPath);
				out.ModTag = NameMatch::ModTagFromBlueprint(out.BlueprintPath);

				if (out.DisplayName.empty())
				{
					out.DisplayName = out.ClassName.empty() ? "Unknown Creature" : out.ClassName;
				}

				return true;
			}

			// Walks the loaded actor list. This is deliberately the plain,
			// version-stable path rather than an octree overlap query: it works on
			// every ServerAPI build, and the per-actor cost is a class check plus a
			// distance compare.
			template <typename Visitor>
			void ForEachDino(Visitor&& visit)
			{
				UWorld* world = ArkApi::GetApiUtils().GetWorld();
				if (world == nullptr)
				{
					return;
				}

				ULevel* level = world->PersistentLevelField();
				if (level == nullptr)
				{
					return;
				}

				TArray<AActor*>& actors = level->ActorsField();
				UClass* dinoClass = APrimalDinoCharacter::GetPrivateStaticClass();

				for (AActor* actor : actors)
				{
					if (actor == nullptr || !actor->IsA(dinoClass))
					{
						continue;
					}

					if (!visit(static_cast<APrimalDinoCharacter*>(actor)))
					{
						return;
					}
				}
			}
		}

		std::vector<DinoInfo> ScanDinos(const Vec3& origin, double radiusCm)
		{
			std::vector<DinoInfo> found;

			ForEachDino([&](APrimalDinoCharacter* dino)
			{
				DinoInfo info;
				if (!FillDinoInfo(dino, info))
				{
					return true;
				}

				info.DistanceCm = Geo::Distance3D(origin, info.Location);
				if (radiusCm > 0.0 && info.DistanceCm > radiusCm)
				{
					return true;
				}

				found.push_back(info);
				return true;
			});

			return found;
		}

		bool FindDinoById(uint64_t dinoId, DinoInfo& out)
		{
			bool resolved = false;

			ForEachDino([&](APrimalDinoCharacter* dino)
			{
				if (DinoId(dino) != dinoId)
				{
					return true;
				}

				resolved = FillDinoInfo(dino, out);
				return false;
			});

			return resolved;
		}

		Vec3 GetPlayerLocation(AShooterPlayerController* player)
		{
			if (player == nullptr)
			{
				return Vec3{};
			}
			return ToVec3(ArkApi::GetApiUtils().GetPosition(player));
		}

		double GetPlayerYaw(AShooterPlayerController* player)
		{
			if (player == nullptr)
			{
				return 0.0;
			}

			// ARK yaw is 0 = +X = east and increases clockwise, while our bearings
			// are 0 = north, so shift by 90 degrees.
			const FRotator rotation = player->ControlRotationField();
			return Geo::NormalizeDegrees(static_cast<double>(rotation.Yaw) + 90.0);
		}

		uint64_t GetPlayerId(AShooterPlayerController* player)
		{
			if (player == nullptr)
			{
				return 0;
			}
			return ArkApi::GetApiUtils().GetSteamIdFromController(player);
		}

		AShooterPlayerController* FindPlayer(uint64_t playerId)
		{
			if (playerId == 0)
			{
				return nullptr;
			}
			return ArkApi::GetApiUtils().FindPlayerFromSteamId(playerId);
		}

		bool IsAdmin(AShooterPlayerController* player)
		{
			return player != nullptr && player->bIsAdmin()();
		}

		void SendChat(AShooterPlayerController* player, const std::string& text)
		{
			if (player == nullptr)
			{
				return;
			}

			ArkApi::GetApiUtils().SendChatMessage(player, FString("ArkFind"), *ToFString(text));
		}

		void SendNotification(AShooterPlayerController* player, const std::string& text, float durationSeconds)
		{
			if (player == nullptr)
			{
				return;
			}

			// SendNotification takes an FLinearColor, not an FColor.
			ArkApi::GetApiUtils().SendNotification(player, FLinearColor(0.35f, 1.0f, 0.45f, 1.0f),
				1.2f, durationSeconds, nullptr, *ToFString(text));
		}

		std::string PluginConfigPath()
		{
			return ArkApi::Tools::Utf8Encode(ArkApi::Tools::GetCurrentDir())
				+ "/ArkApi/Plugins/ArkFind/config.json";
		}
	}
}
