#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "DinoInfo.h"

// Everything in this file talks to the ARK SDK. It is the ONLY version sensitive
// part of the plugin: if a future ServerAPI release renames a field accessor,
// Scanner.cpp is the file to patch. Nothing else in the plugin includes Ark.h.
class AShooterPlayerController;

namespace ArkFind
{
	namespace Sdk
	{
		// Wild + tamed dinos currently loaded in the world, within `radiusCm` of
		// `origin`. A radius of 0 means "no limit".
		std::vector<DinoInfo> ScanDinos(const Vec3& origin, double radiusCm);

		// Re-resolves a previously seen dino by its persistent dino id so that
		// tracking keeps working as the actor moves. Returns false when the dino
		// is no longer loaded (out of streaming range) or is dead.
		bool FindDinoById(uint64_t dinoId, DinoInfo& out);

		Vec3 GetPlayerLocation(AShooterPlayerController* player);

		// Yaw in degrees, 0 = north, matching Geo::BearingDegrees.
		double GetPlayerYaw(AShooterPlayerController* player);

		uint64_t GetPlayerId(AShooterPlayerController* player);

		// nullptr when the player is no longer connected.
		AShooterPlayerController* FindPlayer(uint64_t playerId);

		bool IsAdmin(AShooterPlayerController* player);

		void SendChat(AShooterPlayerController* player, const std::string& text);
		void SendNotification(AShooterPlayerController* player, const std::string& text, float durationSeconds);

		// Absolute path of the plugin's own directory, used to locate config.json.
		std::string PluginConfigPath();
	}
}
