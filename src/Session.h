#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "DinoInfo.h"
#include "Geo.h"

namespace ArkFind
{
	struct SearchOptions
	{
		double RadiusCm = 200000.0;   // 2000m; 0 = no distance limit
		int MaxResults = 15;
		double MatchThreshold = 0.35;
		bool IncludeTamed = false;
		int MinLevel = 0;
		int MaxLevel = 0;             // 0 = no upper bound
	};

	// One numbered entry in the "here is what I found" list.
	struct SearchHit
	{
		DinoInfo Dino;
		double MatchScore = 0.0;
	};

	// What the player is currently hunting.
	struct TrackedTarget
	{
		bool Active = false;
		uint64_t ActorId = 0;
		std::string DisplayName;
		std::string BlueprintPath;
		Vec3 LastKnownLocation;
		int TicksSinceSeen = 0;
		bool Arrived = false;
	};

	struct PlayerSession
	{
		std::string LastQuery;
		std::vector<SearchHit> LastResults;
		TrackedTarget Target;
	};

	// Filters, scores and ranks a raw scan against a query.
	//
	// An empty query returns everything in range, which is what `/finddino` with
	// no argument does.
	std::vector<SearchHit> RankResults(const std::vector<DinoInfo>& dinos,
		const Vec3& playerLocation,
		const std::string& query,
		const SearchOptions& options);

	// Formats one line of the numbered pick list.
	std::string FormatHitLine(int index, const SearchHit& hit);

	struct DirectionOptions
	{
		double ArrivalRadiusCm = 1500.0;   // 15m
		double VerticalToleranceCm = 800.0;
		bool ShowMapCoords = true;
		Geo::MapGpsSettings Gps;
	};

	// The live guidance line, e.g.
	//   "^ Rex - 412m straight ahead (N), above you [lat 41.2 lon 63.8]"
	std::string FormatDirection(const TrackedTarget& target,
		const Vec3& playerLocation,
		double playerYawDeg,
		const DirectionOptions& options);

	// True once the player is inside the arrival radius of the target.
	bool HasArrived(const TrackedTarget& target, const Vec3& playerLocation, double arrivalRadiusCm);

	// Keyed by ARK's SteamId / EOS id.
	using SessionStore = std::unordered_map<uint64_t, PlayerSession>;

	// Parses "3" / "#3" / "pick 3" into a zero based index. Returns -1 when the
	// argument is not a valid selection for a list of `resultCount` entries.
	int ParseSelection(const std::string& argument, size_t resultCount);
}
