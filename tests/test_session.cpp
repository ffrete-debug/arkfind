#include "test_framework.h"

#include "Session.h"

using ArkFind::DinoInfo;
using ArkFind::DirectionOptions;
using ArkFind::SearchHit;
using ArkFind::SearchOptions;
using ArkFind::TrackedTarget;
using ArkFind::Vec3;

namespace
{
	Vec3 At(double x, double y, double z = 0.0)
	{
		Vec3 v;
		v.X = x;
		v.Y = y;
		v.Z = z;
		return v;
	}

	// A plain wild dino `x` centimetres due east of the origin.
	DinoInfo MakeDino(const std::string& displayName, double x, int level = 100)
	{
		DinoInfo dino;
		dino.DisplayName = displayName;
		dino.Location = At(x, 0.0, 0.0);
		dino.Level = level;
		dino.IsAlive = true;
		dino.IsTamed = false;
		dino.IsFemale = false;
		return dino;
	}

	std::vector<std::string> NamesOf(const std::vector<SearchHit>& hits)
	{
		std::vector<std::string> names;
		names.reserve(hits.size());
		for (const SearchHit& hit : hits)
		{
			names.push_back(hit.Dino.DisplayName);
		}
		return names;
	}

	std::string Joined(const std::vector<SearchHit>& hits)
	{
		std::string out;
		for (const std::string& name : NamesOf(hits))
		{
			if (!out.empty())
			{
				out += ",";
			}
			out += name;
		}
		return out;
	}

	const std::string VoidBlueprint =
		"/Game/Mods/AE/Dinos/Wyvern/Void_Character_BP.Void_Character_BP_C";

	DinoInfo MakeModDino(double x)
	{
		DinoInfo dino = MakeDino("Voidwyrm", x, 300);
		dino.ClassName = "Void_Character_BP";
		dino.BlueprintPath = VoidBlueprint;
		dino.ModTag = "AE";
		return dino;
	}
}

// ============================================================== RankResults

TEST_CASE(Session_RankResults_EmptyQueryReturnsEverythingSortedByDistance)
{
	const std::vector<DinoInfo> dinos = {
		MakeDino("Rex", 50000),
		MakeDino("Dodo", 10000),
		MakeDino("Parasaur", 30000)
	};

	const SearchOptions options;
	const std::vector<SearchHit> hits = ArkFind::RankResults(dinos, At(0, 0, 0), "", options);

	CHECK_EQ(hits.size(), static_cast<size_t>(3));
	CHECK_STR_EQ(Joined(hits), "Dodo,Parasaur,Rex");

	// An empty query gives every survivor the same full score.
	for (const SearchHit& hit : hits)
	{
		CHECK_NEAR(hit.MatchScore, 1.0, 1e-12);
	}

	// The hit carries the distance measured from the player, not whatever the
	// scanner happened to leave in the struct.
	CHECK_NEAR(hits[0].Dino.DistanceCm, 10000.0, 1e-9);
	CHECK_NEAR(hits[1].Dino.DistanceCm, 30000.0, 1e-9);
	CHECK_NEAR(hits[2].Dino.DistanceCm, 50000.0, 1e-9);
}

TEST_CASE(Session_RankResults_WhitespaceOnlyQueryCountsAsEmpty)
{
	const std::vector<DinoInfo> dinos = { MakeDino("Rex", 10000), MakeDino("Dodo", 20000) };
	const SearchOptions options;

	CHECK_EQ(ArkFind::RankResults(dinos, At(0, 0, 0), "   ", options).size(),
		static_cast<size_t>(2));
	CHECK_EQ(ArkFind::RankResults(dinos, At(0, 0, 0), "-- ", options).size(),
		static_cast<size_t>(2));
}

TEST_CASE(Session_RankResults_DistanceIsMeasuredFromThePlayer)
{
	const std::vector<DinoInfo> dinos = { MakeDino("Rex", 10000) };
	SearchOptions options;
	options.RadiusCm = 0.0;

	// Player standing 4000cm east: the Rex is now only 6000cm away.
	const std::vector<SearchHit> hits = ArkFind::RankResults(dinos, At(4000, 0, 0), "", options);
	CHECK_EQ(hits.size(), static_cast<size_t>(1));
	CHECK_NEAR(hits[0].Dino.DistanceCm, 6000.0, 1e-9);
}

TEST_CASE(Session_RankResults_FiltersByRadius)
{
	const std::vector<DinoInfo> dinos = {
		MakeDino("Near", 10000),
		MakeDino("Far", 300000)
	};

	SearchOptions options;
	options.RadiusCm = 200000.0;
	CHECK_STR_EQ(Joined(ArkFind::RankResults(dinos, At(0, 0, 0), "", options)), "Near");

	// The radius is inclusive.
	options.RadiusCm = 300000.0;
	CHECK_STR_EQ(Joined(ArkFind::RankResults(dinos, At(0, 0, 0), "", options)), "Near,Far");

	options.RadiusCm = 299999.0;
	CHECK_STR_EQ(Joined(ArkFind::RankResults(dinos, At(0, 0, 0), "", options)), "Near");

	// Radius 0 means "no limit", not "nothing in range".
	options.RadiusCm = 0.0;
	CHECK_STR_EQ(Joined(ArkFind::RankResults(dinos, At(0, 0, 0), "", options)), "Near,Far");
}

TEST_CASE(Session_RankResults_RadiusUsesThreeDimensionalDistance)
{
	DinoInfo overhead = MakeDino("Overhead", 0.0);
	overhead.Location = At(0, 0, 300000);

	std::vector<DinoInfo> dinos = { overhead };

	SearchOptions options;
	options.RadiusCm = 200000.0;
	CHECK_EQ(ArkFind::RankResults(dinos, At(0, 0, 0), "", options).size(),
		static_cast<size_t>(0));

	options.RadiusCm = 400000.0;
	const std::vector<SearchHit> hits = ArkFind::RankResults(dinos, At(0, 0, 0), "", options);
	CHECK_EQ(hits.size(), static_cast<size_t>(1));
	CHECK_NEAR(hits[0].Dino.DistanceCm, 300000.0, 1e-9);
}

TEST_CASE(Session_RankResults_SkipsDeadDinos)
{
	DinoInfo dead = MakeDino("Corpse", 5000);
	dead.IsAlive = false;

	const std::vector<DinoInfo> dinos = { dead, MakeDino("Rex", 10000) };
	const SearchOptions options;

	CHECK_STR_EQ(Joined(ArkFind::RankResults(dinos, At(0, 0, 0), "", options)), "Rex");
}

TEST_CASE(Session_RankResults_TamedFilter)
{
	DinoInfo tamed = MakeDino("Bob", 5000);
	tamed.IsTamed = true;

	const std::vector<DinoInfo> dinos = { tamed, MakeDino("Rex", 10000) };

	SearchOptions options;
	options.IncludeTamed = false;
	CHECK_STR_EQ(Joined(ArkFind::RankResults(dinos, At(0, 0, 0), "", options)), "Rex");

	options.IncludeTamed = true;
	CHECK_STR_EQ(Joined(ArkFind::RankResults(dinos, At(0, 0, 0), "", options)), "Bob,Rex");
}

TEST_CASE(Session_RankResults_LevelFilters)
{
	const std::vector<DinoInfo> dinos = {
		MakeDino("L5", 1000, 5),
		MakeDino("L50", 2000, 50),
		MakeDino("L150", 3000, 150),
		MakeDino("L400", 4000, 400)
	};

	SearchOptions options;

	// 0/0 means unbounded in both directions.
	CHECK_STR_EQ(Joined(ArkFind::RankResults(dinos, At(0, 0, 0), "", options)),
		"L5,L50,L150,L400");

	options.MinLevel = 50;
	CHECK_STR_EQ(Joined(ArkFind::RankResults(dinos, At(0, 0, 0), "", options)),
		"L50,L150,L400");

	options.MinLevel = 0;
	options.MaxLevel = 150;
	CHECK_STR_EQ(Joined(ArkFind::RankResults(dinos, At(0, 0, 0), "", options)),
		"L5,L50,L150");

	options.MinLevel = 50;
	options.MaxLevel = 150;
	CHECK_STR_EQ(Joined(ArkFind::RankResults(dinos, At(0, 0, 0), "", options)),
		"L50,L150");

	// Bounds are inclusive on both ends.
	options.MinLevel = 150;
	options.MaxLevel = 150;
	CHECK_STR_EQ(Joined(ArkFind::RankResults(dinos, At(0, 0, 0), "", options)), "L150");

	options.MinLevel = 151;
	options.MaxLevel = 0;
	CHECK_STR_EQ(Joined(ArkFind::RankResults(dinos, At(0, 0, 0), "", options)), "L400");
}

TEST_CASE(Session_RankResults_OrdersByScoreThenDistance)
{
	std::vector<DinoInfo> dinos = {
		MakeDino("Rex", 50000),    // exact match, farther
		MakeDino("Rexy", 100),     // weaker match, but almost on top of the player
		MakeDino("Rex", 10000)     // exact match, nearer
	};
	dinos[0].ActorId = 1;
	dinos[1].ActorId = 2;
	dinos[2].ActorId = 3;

	const SearchOptions options;
	const std::vector<SearchHit> hits = ArkFind::RankResults(dinos, At(0, 0, 0), "rex", options);

	CHECK_EQ(hits.size(), static_cast<size_t>(3));
	// Best name score first; within an equal score, nearest first.
	CHECK_STR_EQ(Joined(hits), "Rex,Rex,Rexy");

	// Identity is preserved, so the caller can tell the two Rexes apart.
	CHECK_EQ(hits[0].Dino.ActorId, static_cast<uint64_t>(3));   // near exact match
	CHECK_EQ(hits[1].Dino.ActorId, static_cast<uint64_t>(1));   // far exact match
	CHECK_EQ(hits[2].Dino.ActorId, static_cast<uint64_t>(2));   // weaker match

	CHECK_NEAR(hits[0].MatchScore, 1.0, 1e-12);
	CHECK_NEAR(hits[1].MatchScore, 1.0, 1e-12);
	CHECK(hits[2].MatchScore < 1.0);
	CHECK(hits[2].MatchScore > 0.85);
	// The nearer dino really is the weaker match, so score beat distance.
	CHECK(hits[2].Dino.DistanceCm < hits[0].Dino.DistanceCm);
}

TEST_CASE(Session_RankResults_MatchThreshold)
{
	const std::vector<DinoInfo> dinos = {
		MakeDino("Rex", 10000),
		MakeDino("Rexy", 20000),
		MakeDino("Dodo", 30000)
	};

	SearchOptions options;

	// Default threshold keeps both rex-ish names and drops the Dodo.
	CHECK_STR_EQ(Joined(ArkFind::RankResults(dinos, At(0, 0, 0), "rex", options)), "Rex,Rexy");

	// A strict threshold only admits the exact name.
	options.MatchThreshold = 0.95;
	CHECK_STR_EQ(Joined(ArkFind::RankResults(dinos, At(0, 0, 0), "rex", options)), "Rex");

	// Nothing matches at all.
	options.MatchThreshold = 0.35;
	CHECK_EQ(ArkFind::RankResults(dinos, At(0, 0, 0), "zzzz", options).size(),
		static_cast<size_t>(0));
}

TEST_CASE(Session_RankResults_MaxResultsTruncatesToTheBest)
{
	const std::vector<DinoInfo> dinos = {
		MakeDino("A", 5000),
		MakeDino("B", 4000),
		MakeDino("C", 3000),
		MakeDino("D", 2000),
		MakeDino("E", 1000)
	};

	SearchOptions options;
	options.MaxResults = 2;
	CHECK_STR_EQ(Joined(ArkFind::RankResults(dinos, At(0, 0, 0), "", options)), "E,D");

	options.MaxResults = 5;
	CHECK_EQ(ArkFind::RankResults(dinos, At(0, 0, 0), "", options).size(),
		static_cast<size_t>(5));

	options.MaxResults = 99;
	CHECK_EQ(ArkFind::RankResults(dinos, At(0, 0, 0), "", options).size(),
		static_cast<size_t>(5));

	// 0 means "no cap".
	options.MaxResults = 0;
	CHECK_EQ(ArkFind::RankResults(dinos, At(0, 0, 0), "", options).size(),
		static_cast<size_t>(5));
}

TEST_CASE(Session_RankResults_FindsModDinoByNameClassBlueprintOrTag)
{
	const std::vector<DinoInfo> dinos = { MakeModDino(10000) };
	const SearchOptions options;
	const Vec3 player = At(0, 0, 0);

	CHECK_EQ(ArkFind::RankResults(dinos, player, "voidwyrm", options).size(),
		static_cast<size_t>(1));
	CHECK_EQ(ArkFind::RankResults(dinos, player, "void", options).size(),
		static_cast<size_t>(1));
	CHECK_EQ(ArkFind::RankResults(dinos, player, "Void_Character_BP", options).size(),
		static_cast<size_t>(1));
	// Searchable by mod tag alone, and by "<tag> <name>".
	CHECK_EQ(ArkFind::RankResults(dinos, player, "ae", options).size(),
		static_cast<size_t>(1));
	CHECK_EQ(ArkFind::RankResults(dinos, player, "AE Voidwyrm", options).size(),
		static_cast<size_t>(1));
	// And not by an unrelated species.
	CHECK_EQ(ArkFind::RankResults(dinos, player, "stegosaurus", options).size(),
		static_cast<size_t>(0));
}

TEST_CASE(Session_RankResults_EmptyInput)
{
	const std::vector<DinoInfo> none;
	const SearchOptions options;
	CHECK_EQ(ArkFind::RankResults(none, At(0, 0, 0), "rex", options).size(),
		static_cast<size_t>(0));
	CHECK_EQ(ArkFind::RankResults(none, At(0, 0, 0), "", options).size(),
		static_cast<size_t>(0));
}

// ============================================================ FormatHitLine

TEST_CASE(Session_FormatHitLine_WildMaleVanilla)
{
	SearchHit hit;
	hit.Dino = MakeDino("Rex", 0.0, 145);
	hit.Dino.DistanceCm = 41200.0;

	CHECK_STR_EQ(ArkFind::FormatHitLine(1, hit), "1) Rex lvl 145 M - 412m");
}

TEST_CASE(Session_FormatHitLine_FemaleTamedModded)
{
	SearchHit hit;
	hit.Dino = MakeModDino(0.0);
	hit.Dino.Level = 300;
	hit.Dino.IsFemale = true;
	hit.Dino.IsTamed = true;
	hit.Dino.DistanceCm = 150000.0;

	CHECK_STR_EQ(ArkFind::FormatHitLine(7, hit),
		"7) Voidwyrm [AE] lvl 300 F (tamed) - 1.50km");
}

TEST_CASE(Session_FormatHitLine_DistanceFormatting)
{
	SearchHit hit;
	hit.Dino = MakeDino("Dodo", 0.0, 1);

	hit.Dino.DistanceCm = 0.0;
	CHECK_STR_EQ(ArkFind::FormatHitLine(1, hit), "1) Dodo lvl 1 M - 0m");

	hit.Dino.DistanceCm = 12300.0;               // 123 m
	CHECK_STR_EQ(ArkFind::FormatHitLine(1, hit), "1) Dodo lvl 1 M - 123m");

	hit.Dino.DistanceCm = 99900.0;               // 999 m, still metres
	CHECK_STR_EQ(ArkFind::FormatHitLine(1, hit), "1) Dodo lvl 1 M - 999m");

	hit.Dino.DistanceCm = 100000.0;              // exactly 1000 m -> kilometres
	CHECK_STR_EQ(ArkFind::FormatHitLine(1, hit), "1) Dodo lvl 1 M - 1.00km");

	hit.Dino.DistanceCm = 234500.0;              // 2345 m
	CHECK_STR_EQ(ArkFind::FormatHitLine(1, hit), "1) Dodo lvl 1 M - 2.35km");
}

TEST_CASE(Session_FormatHitLine_IndexIsPassedThrough)
{
	SearchHit hit;
	hit.Dino = MakeDino("Rex", 0.0, 5);
	hit.Dino.DistanceCm = 1000.0;

	CHECK_STR_EQ(ArkFind::FormatHitLine(1, hit), "1) Rex lvl 5 M - 10m");
	CHECK_STR_EQ(ArkFind::FormatHitLine(12, hit), "12) Rex lvl 5 M - 10m");
}

TEST_CASE(Session_FormatHitLine_ModTagOmittedForVanilla)
{
	SearchHit hit;
	hit.Dino = MakeDino("Rex", 0.0, 10);
	hit.Dino.ModTag = "";
	hit.Dino.DistanceCm = 100.0;

	const std::string line = ArkFind::FormatHitLine(3, hit);
	CHECK_STR_EQ(line, "3) Rex lvl 10 M - 1m");
	CHECK(line.find('[') == std::string::npos);
	CHECK(line.find("(tamed)") == std::string::npos);
}

// =========================================================== FormatDirection

namespace
{
	TrackedTarget MakeTarget(const std::string& name, const Vec3& location)
	{
		TrackedTarget target;
		target.Active = true;
		target.ActorId = 42;
		target.DisplayName = name;
		target.BlueprintPath = VoidBlueprint;
		target.LastKnownLocation = location;
		return target;
	}

	DirectionOptions PlainOptions()
	{
		DirectionOptions options;
		options.ArrivalRadiusCm = 1500.0;
		options.VerticalToleranceCm = 800.0;
		options.ShowMapCoords = false;
		return options;
	}
}

TEST_CASE(Session_FormatDirection_StraightAheadDueNorth)
{
	// North is -Y in ARK, and the player is facing north (yaw 0).
	const TrackedTarget target = MakeTarget("Rex", At(0, -41200, 0));

	CHECK_STR_EQ(ArkFind::FormatDirection(target, At(0, 0, 0), 0.0, PlainOptions()),
		"^ Rex - 412m straight ahead (N)");
}

TEST_CASE(Session_FormatDirection_TargetToTheSides)
{
	const DirectionOptions options = PlainOptions();
	const Vec3 player = At(0, 0, 0);

	// Due east while facing north.
	CHECK_STR_EQ(ArkFind::FormatDirection(MakeTarget("Rex", At(41200, 0, 0)), player, 0.0, options),
		"> Rex - 412m to your right (E)");

	// Due west while facing north.
	CHECK_STR_EQ(ArkFind::FormatDirection(MakeTarget("Rex", At(-41200, 0, 0)), player, 0.0, options),
		"< Rex - 412m to your left (W)");

	// Due north while facing east: the target is now on the left.
	CHECK_STR_EQ(ArkFind::FormatDirection(MakeTarget("Rex", At(0, -41200, 0)), player, 90.0, options),
		"< Rex - 412m to your left (N)");
}

TEST_CASE(Session_FormatDirection_BehindThePlayer)
{
	const TrackedTarget target = MakeTarget("Rex", At(0, -41200, 0));

	// Facing due south while the Rex is due north.
	CHECK_STR_EQ(ArkFind::FormatDirection(target, At(0, 0, 0), 180.0, PlainOptions()),
		"v Rex - 412m behind you (N)");
}

TEST_CASE(Session_FormatDirection_DiagonalUsesCardinalAndSoftTurn)
{
	// Target to the south-east while facing north.
	const TrackedTarget target = MakeTarget("Rex", At(30000, 30000, 0));
	const std::string line = ArkFind::FormatDirection(target, At(0, 0, 0), 0.0, PlainOptions());

	// sqrt(2) * 300 m = 424.26 m -> "424m"; bearing 135 -> SE; relative 135 -> hard right.
	CHECK_STR_EQ(line, "\\v Rex - 424m hard right (SE)");
}

TEST_CASE(Session_FormatDirection_VerticalHints)
{
	const DirectionOptions options = PlainOptions();
	const Vec3 player = At(0, 0, 0);

	// 50 m up is well past the 8 m tolerance.
	CHECK_STR_EQ(ArkFind::FormatDirection(MakeTarget("Rex", At(30000, 0, 5000)), player, 90.0, options),
		"^ Rex - 304m straight ahead (E), above you");

	CHECK_STR_EQ(ArkFind::FormatDirection(MakeTarget("Rex", At(30000, 0, -5000)), player, 90.0, options),
		"^ Rex - 304m straight ahead (E), below you");

	// Inside the tolerance there is no vertical clause at all.
	const std::string level =
		ArkFind::FormatDirection(MakeTarget("Rex", At(30000, 0, 800)), player, 90.0, options);
	CHECK_STR_EQ(level, "^ Rex - 300m straight ahead (E)");
	CHECK(level.find("above") == std::string::npos);
	CHECK(level.find("below") == std::string::npos);
}

TEST_CASE(Session_FormatDirection_MapCoordsAppendedWhenEnabled)
{
	DirectionOptions options = PlainOptions();
	options.ShowMapCoords = true;
	options.Gps.LatOrigin = 0.0;
	options.Gps.LonOrigin = 0.0;
	options.Gps.LatScale = 100.0;   // -> divide centimetres by 10000, times 100
	options.Gps.LonScale = 100.0;

	// Due south (+Y) while facing north, so the target is straight behind.
	const TrackedTarget target = MakeTarget("Rex", At(0, 5000, 0));
	CHECK_STR_EQ(ArkFind::FormatDirection(target, At(0, 0, 0), 0.0, options),
		"v Rex - 50m behind you (S) [lat 50.0 lon 0.0]");
}

TEST_CASE(Session_FormatDirection_MapCoordsWithDefaultGpsSettings)
{
	DirectionOptions options = PlainOptions();
	options.ShowMapCoords = true;   // default Gps: origin -342900, 6858 cm/unit

	const TrackedTarget target = MakeTarget("Rex", At(0, -41200, 0));
	CHECK_STR_EQ(ArkFind::FormatDirection(target, At(0, 0, 0), 0.0, options),
		"^ Rex - 412m straight ahead (N) [lat 44.0 lon 50.0]");
}

TEST_CASE(Session_FormatDirection_FoundBranchInsideArrivalRadius)
{
	DirectionOptions options = PlainOptions();
	options.ArrivalRadiusCm = 1500.0;

	// 10 m away, inside the 15 m arrival radius.
	const TrackedTarget target = MakeTarget("Rex", At(1000, 0, 0));
	const std::string line = ArkFind::FormatDirection(target, At(0, 0, 0), 0.0, options);

	CHECK_STR_EQ(line, "FOUND Rex - it is right here (10m).");
	// The arrival message replaces the whole guidance line.
	CHECK(line.find("straight ahead") == std::string::npos);
	CHECK(line.find('(') != std::string::npos);

	// The boundary itself counts as arrival.
	CHECK_STR_EQ(ArkFind::FormatDirection(MakeTarget("Rex", At(1500, 0, 0)), At(0, 0, 0), 0.0, options),
		"FOUND Rex - it is right here (15m).");

	// One centimetre further out and the guidance line comes back.
	const std::string outside =
		ArkFind::FormatDirection(MakeTarget("Rex", At(1501, 0, 0)), At(0, 0, 0), 0.0, options);
	CHECK(outside.find("FOUND") == std::string::npos);
	CHECK_STR_EQ(outside, "> Rex - 15m to your right (E)");

	// Standing exactly on top of the target still uses the FOUND branch.
	CHECK_STR_EQ(ArkFind::FormatDirection(MakeTarget("Rex", At(0, 0, 0)), At(0, 0, 0), 0.0, options),
		"FOUND Rex - it is right here (0m).");
}

TEST_CASE(Session_FormatDirection_ArrivalRadiusIsThreeDimensional)
{
	DirectionOptions options = PlainOptions();
	options.ArrivalRadiusCm = 1500.0;

	// Directly overhead, but 20 m up: not arrived.
	const std::string line =
		ArkFind::FormatDirection(MakeTarget("Rex", At(0, 0, 2000)), At(0, 0, 0), 0.0, options);
	CHECK(line.find("FOUND") == std::string::npos);
	// Straight up has no horizontal bearing, so it degenerates to north/ahead.
	CHECK_STR_EQ(line, "^ Rex - 20m straight ahead (N), above you");
}

// ================================================================ HasArrived

TEST_CASE(Session_HasArrived)
{
	const TrackedTarget target = MakeTarget("Rex", At(1000, 0, 0));

	CHECK(ArkFind::HasArrived(target, At(0, 0, 0), 1500.0));
	CHECK(ArkFind::HasArrived(target, At(0, 0, 0), 1000.0));   // inclusive
	CHECK(!ArkFind::HasArrived(target, At(0, 0, 0), 999.0));
	CHECK(!ArkFind::HasArrived(target, At(0, 0, 0), 0.0));

	// Walking closer flips it.
	CHECK(ArkFind::HasArrived(target, At(900, 0, 0), 200.0));
	CHECK(!ArkFind::HasArrived(target, At(700, 0, 0), 200.0));

	// Standing on the target arrives even with a zero radius.
	CHECK(ArkFind::HasArrived(MakeTarget("Rex", At(0, 0, 0)), At(0, 0, 0), 0.0));
}

TEST_CASE(Session_HasArrived_UsesHeightToo)
{
	const TrackedTarget overhead = MakeTarget("Rex", At(0, 0, 3000));

	CHECK(!ArkFind::HasArrived(overhead, At(0, 0, 0), 1500.0));
	CHECK(ArkFind::HasArrived(overhead, At(0, 0, 0), 3000.0));
	CHECK(ArkFind::HasArrived(overhead, At(0, 0, 2500), 500.0));
}

TEST_CASE(Session_HasArrived_AgreesWithFormatDirection)
{
	DirectionOptions options = PlainOptions();
	options.ArrivalRadiusCm = 1500.0;

	const TrackedTarget inside = MakeTarget("Rex", At(1400, 0, 0));
	const TrackedTarget outside = MakeTarget("Rex", At(1600, 0, 0));

	CHECK(ArkFind::HasArrived(inside, At(0, 0, 0), options.ArrivalRadiusCm));
	CHECK(ArkFind::FormatDirection(inside, At(0, 0, 0), 0.0, options).find("FOUND")
		== static_cast<size_t>(0));

	CHECK(!ArkFind::HasArrived(outside, At(0, 0, 0), options.ArrivalRadiusCm));
	CHECK(ArkFind::FormatDirection(outside, At(0, 0, 0), 0.0, options).find("FOUND")
		== std::string::npos);
}

// ============================================================ ParseSelection

TEST_CASE(Session_ParseSelection_AcceptsPlainNumbers)
{
	CHECK_EQ(ArkFind::ParseSelection("1", 5), 0);
	CHECK_EQ(ArkFind::ParseSelection("3", 5), 2);
	CHECK_EQ(ArkFind::ParseSelection("5", 5), 4);
	CHECK_EQ(ArkFind::ParseSelection("15", 20), 14);
}

TEST_CASE(Session_ParseSelection_AcceptsDecoratedForms)
{
	CHECK_EQ(ArkFind::ParseSelection("#3", 5), 2);
	CHECK_EQ(ArkFind::ParseSelection("pick 3", 5), 2);
	CHECK_EQ(ArkFind::ParseSelection("pick #3", 5), 2);
	CHECK_EQ(ArkFind::ParseSelection("  3  ", 5), 2);
	CHECK_EQ(ArkFind::ParseSelection("3)", 5), 2);
	// Only the first run of digits is used; trailing junk is ignored.
	CHECK_EQ(ArkFind::ParseSelection("3 rex", 5), 2);
	CHECK_EQ(ArkFind::ParseSelection("2abc", 5), 1);
}

TEST_CASE(Session_ParseSelection_RejectsZeroAndNonNumbers)
{
	CHECK_EQ(ArkFind::ParseSelection("0", 5), -1);
	CHECK_EQ(ArkFind::ParseSelection("#0", 5), -1);
	CHECK_EQ(ArkFind::ParseSelection("abc", 5), -1);
	CHECK_EQ(ArkFind::ParseSelection("", 5), -1);
	CHECK_EQ(ArkFind::ParseSelection("   ", 5), -1);
	CHECK_EQ(ArkFind::ParseSelection("pick", 5), -1);
	CHECK_EQ(ArkFind::ParseSelection("#", 5), -1);
}

TEST_CASE(Session_ParseSelection_RejectsOutOfRange)
{
	CHECK_EQ(ArkFind::ParseSelection("6", 5), -1);
	CHECK_EQ(ArkFind::ParseSelection("99", 5), -1);
	CHECK_EQ(ArkFind::ParseSelection("1", 0), -1);      // nothing to pick from
	CHECK_EQ(ArkFind::ParseSelection("21", 20), -1);
	// More than four digits is refused outright, before any range check.
	CHECK_EQ(ArkFind::ParseSelection("12345", 99999), -1);
	CHECK_EQ(ArkFind::ParseSelection("9999", 99999), 9998);
}

// ------------------------------------------------- Ranking regression tests

namespace
{
	DinoInfo NamedDinoAt(const std::string& displayName, double distanceCm, uint64_t actorId)
	{
		DinoInfo dino;
		dino.DisplayName = displayName;
		dino.ClassName = displayName;
		dino.BlueprintPath = "/Game/PrimalEarth/Dinos/" + displayName + ".x_C";
		dino.Location = At(distanceCm, 0.0, 0.0);
		dino.Level = 100;
		dino.IsAlive = true;
		dino.ActorId = actorId;
		return dino;
	}
}

// Comparing scores with a tolerance made the sort comparator cyclic for these
// three names: each adjacent pair is within the tolerance (so distance decided)
// while the outer pair was not (so score decided). std::stable_sort with a
// cyclic comparator is undefined behaviour, i.e. a live server crash.
TEST_CASE(Session_RankResults_ComparatorStaysTransitive)
{
	const std::vector<DinoInfo> dinos = {
		NamedDinoAt("Rexosaurus", 100.0, 1),
		NamedDinoAt("Rex Alpha Ice", 200.0, 2),
		NamedDinoAt("Rex Alpha Ice Wyvern", 300.0, 3),
	};

	ArkFind::SearchOptions options;
	options.RadiusCm = 0.0;
	options.MaxResults = 0;

	const std::vector<ArkFind::SearchHit> hits = ArkFind::RankResults(dinos, At(0, 0, 0), "rex", options);
	CHECK_EQ(hits.size(), static_cast<size_t>(3));

	// Scores must be non-increasing, which a cyclic comparator cannot guarantee.
	for (size_t i = 1; i < hits.size(); ++i)
	{
		CHECK(hits[i - 1].MatchScore >= hits[i].MatchScore - 1e-9);
	}

	// And the ordering must be antisymmetric and transitive over the same trio.
	auto less = [](const ArkFind::SearchHit& a, const ArkFind::SearchHit& b)
	{
		const long ka = static_cast<long>(a.MatchScore / 0.01 + 0.5);
		const long kb = static_cast<long>(b.MatchScore / 0.01 + 0.5);
		if (ka != kb)
		{
			return ka > kb;
		}
		if (a.Dino.DistanceCm != b.Dino.DistanceCm)
		{
			return a.Dino.DistanceCm < b.Dino.DistanceCm;
		}
		return a.Dino.ActorId < b.Dino.ActorId;
	};

	for (const auto& a : hits)
	{
		for (const auto& b : hits)
		{
			CHECK(!(less(a, b) && less(b, a)));
			for (const auto& c : hits)
			{
				if (less(a, b) && less(b, c))
				{
					CHECK(less(a, c));
				}
			}
		}
	}
}

// MatchThreshold 0.0 is documented as "most permissive", not "match everything".
TEST_CASE(Session_RankResults_ZeroThresholdStillRejectsZeroScores)
{
	const std::vector<DinoInfo> dinos = {
		NamedDinoAt("Dodo", 100.0, 1),
		NamedDinoAt("Parasaur", 200.0, 2),
		NamedDinoAt("Ankylo", 300.0, 3),
	};

	ArkFind::SearchOptions options;
	options.RadiusCm = 0.0;
	options.MaxResults = 0;
	options.MatchThreshold = 0.0;

	CHECK_EQ(ArkFind::RankResults(dinos, At(0, 0, 0), "xyzzy", options).size(), static_cast<size_t>(0));

	// An empty query still means "everything nearby".
	CHECK_EQ(ArkFind::RankResults(dinos, At(0, 0, 0), "", options).size(), static_cast<size_t>(3));
}

// The unit switch and the printed digits have to agree: 999.7m must not print
// as "1000m" one tick before 1000m prints as "1.00km".
TEST_CASE(Session_FormatHitLine_DistanceUnitMatchesPrintedDigits)
{
	auto hitAt = [](double distanceCm, uint64_t actorId)
	{
		ArkFind::SearchHit hit;
		hit.Dino = NamedDinoAt("Rex", distanceCm, actorId);
		hit.Dino.DistanceCm = distanceCm;
		hit.MatchScore = 1.0;
		return hit;
	};

	const ArkFind::SearchHit belowSwitch = hitAt(99940.0, 1);
	const ArkFind::SearchHit atSwitch = hitAt(99970.0, 2);
	const ArkFind::SearchHit aboveSwitch = hitAt(100000.0, 3);

	CHECK(ArkFind::FormatHitLine(1, belowSwitch).find("999m") != std::string::npos);
	CHECK(ArkFind::FormatHitLine(1, atSwitch).find("1.00km") != std::string::npos);
	CHECK(ArkFind::FormatHitLine(1, aboveSwitch).find("1.00km") != std::string::npos);
}
