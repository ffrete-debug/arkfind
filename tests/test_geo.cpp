#include "test_framework.h"

#include "Geo.h"

using ArkFind::Vec3;
namespace Geo = ArkFind::Geo;

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
}

// ---------------------------------------------------------------- distances

TEST_CASE(Geo_Distance2D_IgnoresZ)
{
	CHECK_NEAR(Geo::Distance2D(At(0, 0, 0), At(300, 400, 99999)), 500.0, 1e-9);
	CHECK_NEAR(Geo::Distance2D(At(10, 10, 5), At(10, 10, 5)), 0.0, 1e-9);
	// Direction does not matter.
	CHECK_NEAR(Geo::Distance2D(At(300, 400, 0), At(0, 0, 0)), 500.0, 1e-9);
}

TEST_CASE(Geo_Distance3D_UsesZ)
{
	CHECK_NEAR(Geo::Distance3D(At(0, 0, 0), At(300, 400, 1200)), 1300.0, 1e-9);
	CHECK_NEAR(Geo::Distance3D(At(0, 0, 0), At(0, 0, -750)), 750.0, 1e-9);
	CHECK_NEAR(Geo::Distance3D(At(5, 5, 5), At(5, 5, 5)), 0.0, 1e-9);
}

TEST_CASE(Geo_CmToMeters)
{
	CHECK_NEAR(Geo::CmPerMeter, 100.0, 1e-12);
	CHECK_NEAR(Geo::CmToMeters(15000.0), 150.0, 1e-9);
	CHECK_NEAR(Geo::CmToMeters(0.0), 0.0, 1e-9);
	CHECK_NEAR(Geo::CmToMeters(-2500.0), -25.0, 1e-9);
}

// ------------------------------------------------------------ NormalizeDegrees

TEST_CASE(Geo_NormalizeDegrees_WrapsIntoZeroTo360)
{
	CHECK_NEAR(Geo::NormalizeDegrees(0.0), 0.0, 1e-9);
	CHECK_NEAR(Geo::NormalizeDegrees(45.5), 45.5, 1e-9);
	CHECK_NEAR(Geo::NormalizeDegrees(359.999), 359.999, 1e-9);
	CHECK_NEAR(Geo::NormalizeDegrees(360.0), 0.0, 1e-9);
	CHECK_NEAR(Geo::NormalizeDegrees(370.0), 10.0, 1e-9);
	CHECK_NEAR(Geo::NormalizeDegrees(720.0), 0.0, 1e-9);
	CHECK_NEAR(Geo::NormalizeDegrees(725.0), 5.0, 1e-9);
	CHECK_NEAR(Geo::NormalizeDegrees(-10.0), 350.0, 1e-9);
	CHECK_NEAR(Geo::NormalizeDegrees(-360.0), 0.0, 1e-9);
	CHECK_NEAR(Geo::NormalizeDegrees(-370.0), 350.0, 1e-9);
	CHECK_NEAR(Geo::NormalizeDegrees(-720.5), 359.5, 1e-9);
}

// -------------------------------------------------------------- BearingDegrees
// ARK: +X is east, +Y is south. So "north" is -Y.

TEST_CASE(Geo_BearingDegrees_Cardinals)
{
	const Vec3 origin = At(0, 0);

	CHECK_NEAR(Geo::BearingDegrees(origin, At(0, -1000)), 0.0, 1e-9);    // north = -Y
	CHECK_NEAR(Geo::BearingDegrees(origin, At(1000, 0)), 90.0, 1e-9);    // east  = +X
	CHECK_NEAR(Geo::BearingDegrees(origin, At(0, 1000)), 180.0, 1e-9);   // south = +Y
	CHECK_NEAR(Geo::BearingDegrees(origin, At(-1000, 0)), 270.0, 1e-9);  // west  = -X
}

TEST_CASE(Geo_BearingDegrees_Diagonals)
{
	const Vec3 origin = At(0, 0);

	CHECK_NEAR(Geo::BearingDegrees(origin, At(500, -500)), 45.0, 1e-9);   // NE
	CHECK_NEAR(Geo::BearingDegrees(origin, At(500, 500)), 135.0, 1e-9);   // SE
	CHECK_NEAR(Geo::BearingDegrees(origin, At(-500, 500)), 225.0, 1e-9);  // SW
	CHECK_NEAR(Geo::BearingDegrees(origin, At(-500, -500)), 315.0, 1e-9); // NW
}

TEST_CASE(Geo_BearingDegrees_IgnoresZAndOffsetOrigin)
{
	// Z must not influence the compass bearing.
	CHECK_NEAR(Geo::BearingDegrees(At(0, 0, 0), At(0, -1000, 50000)), 0.0, 1e-9);
	// Bearing is relative to `from`, not to the world origin.
	CHECK_NEAR(Geo::BearingDegrees(At(1000, 1000), At(1000, 0)), 0.0, 1e-9);
	CHECK_NEAR(Geo::BearingDegrees(At(-5000, 7000), At(-4000, 7000)), 90.0, 1e-9);
}

TEST_CASE(Geo_BearingDegrees_SamePointIsZero)
{
	CHECK_NEAR(Geo::BearingDegrees(At(123, -456, 789), At(123, -456, 789)), 0.0, 1e-12);
	// Only X/Y matter for the degenerate check.
	CHECK_NEAR(Geo::BearingDegrees(At(0, 0, 0), At(0, 0, 1000)), 0.0, 1e-12);
}

// -------------------------------------------------------------- RelativeBearing

TEST_CASE(Geo_RelativeBearing_Basics)
{
	CHECK_NEAR(Geo::RelativeBearing(0.0, 0.0), 0.0, 1e-9);
	CHECK_NEAR(Geo::RelativeBearing(90.0, 0.0), 90.0, 1e-9);
	CHECK_NEAR(Geo::RelativeBearing(0.0, 90.0), -90.0, 1e-9);
	CHECK_NEAR(Geo::RelativeBearing(45.0, 30.0), 15.0, 1e-9);
	CHECK_NEAR(Geo::RelativeBearing(30.0, 45.0), -15.0, 1e-9);
}

TEST_CASE(Geo_RelativeBearing_WrapsAround)
{
	// Crossing north in both directions must give the short way round.
	CHECK_NEAR(Geo::RelativeBearing(10.0, 350.0), 20.0, 1e-9);
	CHECK_NEAR(Geo::RelativeBearing(350.0, 10.0), -20.0, 1e-9);
	CHECK_NEAR(Geo::RelativeBearing(5.0, 355.0), 10.0, 1e-9);
	CHECK_NEAR(Geo::RelativeBearing(-10.0, 10.0), -20.0, 1e-9);
	CHECK_NEAR(Geo::RelativeBearing(370.0, 10.0), 0.0, 1e-9);
	CHECK_NEAR(Geo::RelativeBearing(0.0, 720.0), 0.0, 1e-9);
}

TEST_CASE(Geo_RelativeBearing_IsHalfOpenAt180)
{
	// Range is (-180, 180]: exactly opposite resolves to +180, never -180.
	CHECK_NEAR(Geo::RelativeBearing(180.0, 0.0), 180.0, 1e-9);
	CHECK_NEAR(Geo::RelativeBearing(0.0, 180.0), 180.0, 1e-9);
	CHECK_NEAR(Geo::RelativeBearing(270.0, 90.0), 180.0, 1e-9);
	CHECK(Geo::RelativeBearing(0.0, 180.0) > -180.0);

	// Just either side of opposite.
	CHECK_NEAR(Geo::RelativeBearing(179.0, 0.0), 179.0, 1e-9);
	CHECK_NEAR(Geo::RelativeBearing(181.0, 0.0), -179.0, 1e-9);
}

// ------------------------------------------------------------------ CardinalName

TEST_CASE(Geo_CardinalName_Centres)
{
	CHECK_STR_EQ(Geo::CardinalName(0.0), "N");
	CHECK_STR_EQ(Geo::CardinalName(45.0), "NE");
	CHECK_STR_EQ(Geo::CardinalName(90.0), "E");
	CHECK_STR_EQ(Geo::CardinalName(135.0), "SE");
	CHECK_STR_EQ(Geo::CardinalName(180.0), "S");
	CHECK_STR_EQ(Geo::CardinalName(225.0), "SW");
	CHECK_STR_EQ(Geo::CardinalName(270.0), "W");
	CHECK_STR_EQ(Geo::CardinalName(315.0), "NW");
}

TEST_CASE(Geo_CardinalName_SectorBoundaries)
{
	// Each 45 degree sector is centred on its name, so boundaries sit at
	// 22.5 + k*45 and the boundary value belongs to the *higher* sector.
	CHECK_STR_EQ(Geo::CardinalName(22.4), "N");
	CHECK_STR_EQ(Geo::CardinalName(22.5), "NE");
	CHECK_STR_EQ(Geo::CardinalName(67.4), "NE");
	CHECK_STR_EQ(Geo::CardinalName(67.5), "E");
	CHECK_STR_EQ(Geo::CardinalName(112.5), "SE");
	CHECK_STR_EQ(Geo::CardinalName(157.5), "S");
	CHECK_STR_EQ(Geo::CardinalName(202.5), "SW");
	CHECK_STR_EQ(Geo::CardinalName(247.5), "W");
	CHECK_STR_EQ(Geo::CardinalName(292.5), "NW");
	CHECK_STR_EQ(Geo::CardinalName(337.4), "NW");
	// Wrapping back to north, including the modulo-8 fold.
	CHECK_STR_EQ(Geo::CardinalName(337.5), "N");
	CHECK_STR_EQ(Geo::CardinalName(359.9), "N");
	CHECK_STR_EQ(Geo::CardinalName(360.0), "N");
}

TEST_CASE(Geo_CardinalName_NormalizesInput)
{
	CHECK_STR_EQ(Geo::CardinalName(-45.0), "NW");
	CHECK_STR_EQ(Geo::CardinalName(-90.0), "W");
	CHECK_STR_EQ(Geo::CardinalName(450.0), "E");
	CHECK_STR_EQ(Geo::CardinalName(-360.0), "N");
}

// ---------------------------------------------------------------------- TurnHint

TEST_CASE(Geo_TurnHint_Bands)
{
	// |x| <= 12 -> straight ahead
	CHECK_STR_EQ(Geo::TurnHint(0.0), "straight ahead");
	CHECK_STR_EQ(Geo::TurnHint(12.0), "straight ahead");
	CHECK_STR_EQ(Geo::TurnHint(-12.0), "straight ahead");

	// 12 < |x| <= 45 -> slightly
	CHECK_STR_EQ(Geo::TurnHint(12.1), "slightly right");
	CHECK_STR_EQ(Geo::TurnHint(-12.1), "slightly left");
	CHECK_STR_EQ(Geo::TurnHint(45.0), "slightly right");
	CHECK_STR_EQ(Geo::TurnHint(-45.0), "slightly left");

	// 45 < |x| <= 100 -> to your side
	CHECK_STR_EQ(Geo::TurnHint(45.1), "to your right");
	CHECK_STR_EQ(Geo::TurnHint(-45.1), "to your left");
	CHECK_STR_EQ(Geo::TurnHint(90.0), "to your right");
	CHECK_STR_EQ(Geo::TurnHint(100.0), "to your right");
	CHECK_STR_EQ(Geo::TurnHint(-100.0), "to your left");

	// 100 < |x| <= 155 -> hard
	CHECK_STR_EQ(Geo::TurnHint(100.1), "hard right");
	CHECK_STR_EQ(Geo::TurnHint(-100.1), "hard left");
	CHECK_STR_EQ(Geo::TurnHint(155.0), "hard right");
	CHECK_STR_EQ(Geo::TurnHint(-155.0), "hard left");

	// |x| > 155 -> behind you (no left/right)
	CHECK_STR_EQ(Geo::TurnHint(155.1), "behind you");
	CHECK_STR_EQ(Geo::TurnHint(-155.1), "behind you");
	CHECK_STR_EQ(Geo::TurnHint(180.0), "behind you");
	CHECK_STR_EQ(Geo::TurnHint(-180.0), "behind you");
}

// --------------------------------------------------------------------- TurnArrow

TEST_CASE(Geo_TurnArrow_Bands)
{
	// |x| <= 22.5 -> forward
	CHECK_STR_EQ(Geo::TurnArrow(0.0), "^");
	CHECK_STR_EQ(Geo::TurnArrow(22.5), "^");
	CHECK_STR_EQ(Geo::TurnArrow(-22.5), "^");

	// 22.5 < |x| <= 67.5 -> forward diagonal
	CHECK_STR_EQ(Geo::TurnArrow(22.6), "/^");
	CHECK_STR_EQ(Geo::TurnArrow(-22.6), "^\\");
	CHECK_STR_EQ(Geo::TurnArrow(45.0), "/^");
	CHECK_STR_EQ(Geo::TurnArrow(67.5), "/^");
	CHECK_STR_EQ(Geo::TurnArrow(-67.5), "^\\");

	// 67.5 < |x| <= 112.5 -> sideways
	CHECK_STR_EQ(Geo::TurnArrow(67.6), ">");
	CHECK_STR_EQ(Geo::TurnArrow(-67.6), "<");
	CHECK_STR_EQ(Geo::TurnArrow(90.0), ">");
	CHECK_STR_EQ(Geo::TurnArrow(112.5), ">");
	CHECK_STR_EQ(Geo::TurnArrow(-112.5), "<");

	// 112.5 < |x| <= 157.5 -> backward diagonal
	CHECK_STR_EQ(Geo::TurnArrow(112.6), "\\v");
	CHECK_STR_EQ(Geo::TurnArrow(-112.6), "v/");
	CHECK_STR_EQ(Geo::TurnArrow(157.5), "\\v");
	CHECK_STR_EQ(Geo::TurnArrow(-157.5), "v/");

	// |x| > 157.5 -> straight back
	CHECK_STR_EQ(Geo::TurnArrow(157.6), "v");
	CHECK_STR_EQ(Geo::TurnArrow(-157.6), "v");
	CHECK_STR_EQ(Geo::TurnArrow(180.0), "v");
}

// ---------------------------------------------------------------------- Vertical

TEST_CASE(Geo_Vertical_ToleranceIsInclusive)
{
	const Vec3 player = At(0, 0, 0);

	CHECK(Geo::Vertical(player, At(0, 0, 0), 800.0) == Geo::VerticalHint::Level);
	CHECK(Geo::Vertical(player, At(0, 0, 800), 800.0) == Geo::VerticalHint::Level);
	CHECK(Geo::Vertical(player, At(0, 0, -800), 800.0) == Geo::VerticalHint::Level);
	CHECK(Geo::Vertical(player, At(0, 0, 800.01), 800.0) == Geo::VerticalHint::Above);
	CHECK(Geo::Vertical(player, At(0, 0, -800.01), 800.0) == Geo::VerticalHint::Below);
	CHECK(Geo::Vertical(player, At(0, 0, 50000), 800.0) == Geo::VerticalHint::Above);
	CHECK(Geo::Vertical(player, At(0, 0, -50000), 800.0) == Geo::VerticalHint::Below);
}

TEST_CASE(Geo_Vertical_IsRelativeToPlayerZAndIgnoresXY)
{
	CHECK(Geo::Vertical(At(0, 0, 10000), At(99999, -99999, 10000), 100.0) == Geo::VerticalHint::Level);
	CHECK(Geo::Vertical(At(0, 0, 10000), At(0, 0, 12000), 100.0) == Geo::VerticalHint::Above);
	CHECK(Geo::Vertical(At(0, 0, 10000), At(0, 0, 8000), 100.0) == Geo::VerticalHint::Below);

	// Zero tolerance: any difference at all counts.
	CHECK(Geo::Vertical(At(0, 0, 0), At(0, 0, 0), 0.0) == Geo::VerticalHint::Level);
	CHECK(Geo::Vertical(At(0, 0, 0), At(0, 0, 1), 0.0) == Geo::VerticalHint::Above);
	CHECK(Geo::Vertical(At(0, 0, 0), At(0, 0, -1), 0.0) == Geo::VerticalHint::Below);
}

TEST_CASE(Geo_VerticalName)
{
	CHECK_STR_EQ(Geo::VerticalName(Geo::VerticalHint::Above), "above you");
	CHECK_STR_EQ(Geo::VerticalName(Geo::VerticalHint::Below), "below you");
	CHECK_STR_EQ(Geo::VerticalName(Geo::VerticalHint::Level), "");
}

// -------------------------------------------------------------------- ToMapCoords

TEST_CASE(Geo_ToMapCoords_DefaultSettings)
{
	// Defaults: origin -342900 cm, scale 8000 -> divide by 8000*100 cm, times 100.
	const Geo::MapGpsSettings settings;

	const Geo::MapCoords atOrigin = Geo::ToMapCoords(At(0, 0, 0), settings);
	CHECK_NEAR(atOrigin.Lat, 42.8625, 1e-9);
	CHECK_NEAR(atOrigin.Lon, 42.8625, 1e-9);

	// Lat comes from Y, Lon from X.
	const Geo::MapCoords offset = Geo::ToMapCoords(At(0, -342900, 0), settings);
	CHECK_NEAR(offset.Lat, 0.0, 1e-9);
	CHECK_NEAR(offset.Lon, 42.8625, 1e-9);

	const Geo::MapCoords corner = Geo::ToMapCoords(At(-342900, -342900, 0), settings);
	CHECK_NEAR(corner.Lat, 0.0, 1e-9);
	CHECK_NEAR(corner.Lon, 0.0, 1e-9);
}

TEST_CASE(Geo_ToMapCoords_CustomSettingsAndAxisMapping)
{
	Geo::MapGpsSettings settings;
	settings.LatOrigin = 0.0;
	settings.LonOrigin = 0.0;
	settings.LatScale = 100.0;   // denominator becomes 100 * 100 = 10000 cm
	settings.LonScale = 100.0;

	const Geo::MapCoords coords = Geo::ToMapCoords(At(2500, 5000, 12345), settings);
	CHECK_NEAR(coords.Lat, 50.0, 1e-9);   // Y / 100
	CHECK_NEAR(coords.Lon, 25.0, 1e-9);   // X / 100

	// Z never contributes.
	const Geo::MapCoords high = Geo::ToMapCoords(At(2500, 5000, -99999), settings);
	CHECK_NEAR(high.Lat, coords.Lat, 1e-12);
	CHECK_NEAR(high.Lon, coords.Lon, 1e-12);

	// Different scales per axis, and negative coordinates.
	settings.LatScale = 200.0;   // denominator 20000
	const Geo::MapCoords skewed = Geo::ToMapCoords(At(-2500, -5000, 0), settings);
	CHECK_NEAR(skewed.Lat, -25.0, 1e-9);
	CHECK_NEAR(skewed.Lon, -25.0, 1e-9);
}

TEST_CASE(Geo_ToMapCoords_ZeroScaleLeavesAxisAtZero)
{
	Geo::MapGpsSettings settings;
	settings.LatOrigin = 0.0;
	settings.LonOrigin = 0.0;
	settings.LatScale = 0.0;     // guard against divide-by-zero
	settings.LonScale = 100.0;

	const Geo::MapCoords coords = Geo::ToMapCoords(At(2500, 5000, 0), settings);
	CHECK_NEAR(coords.Lat, 0.0, 1e-12);
	CHECK_NEAR(coords.Lon, 25.0, 1e-9);

	settings.LonScale = 0.0;
	const Geo::MapCoords both = Geo::ToMapCoords(At(2500, 5000, 0), settings);
	CHECK_NEAR(both.Lat, 0.0, 1e-12);
	CHECK_NEAR(both.Lon, 0.0, 1e-12);
}

// ------------------------------------------------------ integration of the pieces

TEST_CASE(Geo_BearingToCardinal_RoundTrip)
{
	const Vec3 player = At(0, 0, 0);

	CHECK_STR_EQ(Geo::CardinalName(Geo::BearingDegrees(player, At(0, -10000))), "N");
	CHECK_STR_EQ(Geo::CardinalName(Geo::BearingDegrees(player, At(10000, -10000))), "NE");
	CHECK_STR_EQ(Geo::CardinalName(Geo::BearingDegrees(player, At(10000, 0))), "E");
	CHECK_STR_EQ(Geo::CardinalName(Geo::BearingDegrees(player, At(10000, 10000))), "SE");
	CHECK_STR_EQ(Geo::CardinalName(Geo::BearingDegrees(player, At(0, 10000))), "S");
	CHECK_STR_EQ(Geo::CardinalName(Geo::BearingDegrees(player, At(-10000, 10000))), "SW");
	CHECK_STR_EQ(Geo::CardinalName(Geo::BearingDegrees(player, At(-10000, 0))), "W");
	CHECK_STR_EQ(Geo::CardinalName(Geo::BearingDegrees(player, At(-10000, -10000))), "NW");
}

TEST_CASE(Geo_PlayerFacingEastSeesNorthTargetOnTheLeft)
{
	const Vec3 player = At(0, 0, 0);
	const double bearing = Geo::BearingDegrees(player, At(0, -10000)); // due north
	const double relative = Geo::RelativeBearing(bearing, 90.0);       // facing east

	CHECK_NEAR(relative, -90.0, 1e-9);
	CHECK_STR_EQ(Geo::TurnHint(relative), "to your left");
	CHECK_STR_EQ(Geo::TurnArrow(relative), "<");
}
