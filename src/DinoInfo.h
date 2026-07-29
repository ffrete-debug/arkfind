#pragma once

#include <cstdint>
#include <string>

namespace ArkFind
{
	struct Vec3
	{
		double X = 0.0;
		double Y = 0.0;
		double Z = 0.0;
	};

	// A single dino observed by the scanner.
	//
	// Nothing in this struct depends on the ARK SDK, so every consumer below the
	// scanner (matching, ranking, direction math, session state) can be built and
	// unit tested on any platform.
	struct DinoInfo
	{
		// Human readable name as the game shows it, e.g. "Rex" or a modded
		// "Voidwyrm". Comes from APrimalDinoCharacter::DescriptiveName, so mod
		// creatures are picked up without any hardcoded species list.
		std::string DisplayName;

		// Blueprint path of the creature class. Used as the stable identity for
		// mod dinos and as a fallback when DescriptiveName is empty.
		std::string BlueprintPath;

		// Short class tag derived from the blueprint path, e.g. "Rex_Character_BP".
		std::string ClassName;

		// Mod tag parsed out of the blueprint path ("" for vanilla creatures).
		std::string ModTag;

		Vec3 Location;

		int Level = 0;
		bool IsFemale = false;
		bool IsTamed = false;
		bool IsAlive = true;

		// Runtime id of the actor, so a tracked target survives rescans.
		uint64_t ActorId = 0;

		// Filled in by the scanner relative to the requesting player.
		double DistanceCm = 0.0;
	};
}
