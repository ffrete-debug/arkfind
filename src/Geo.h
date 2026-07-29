#pragma once

#include <string>

#include "DinoInfo.h"

namespace ArkFind
{
	namespace Geo
	{
		// ARK works in centimeters. One "foundation" is 300cm; a meter is 100cm.
		constexpr double CmPerMeter = 100.0;

		struct MapCoords
		{
			double Lat = 0.0;
			double Lon = 0.0;
		};

		// Origin/scale of the in-game GPS readout. Values differ per map, so they
		// come from config rather than being baked in.
		//
		// ToMapCoords works out to (coord - origin) / scale, so `scale` is the
		// number of world centimetres per GPS unit. The defaults are The Island's:
		// 342900 / 6858 = 50.0, i.e. world origin sits at the centre of the map,
		// which is what the in-game GPS shows.
		struct MapGpsSettings
		{
			double LatOrigin = -342900.0;
			double LonOrigin = -342900.0;
			double LatScale = 6858.0;
			double LonScale = 6858.0;
		};

		enum class VerticalHint
		{
			Level,
			Above,
			Below
		};

		double Distance2D(const Vec3& from, const Vec3& to);
		double Distance3D(const Vec3& from, const Vec3& to);

		double CmToMeters(double cm);

		// Compass bearing from `from` to `to`, in degrees clockwise from north,
		// normalized to [0, 360). ARK's +X axis points east and +Y points south,
		// which is why north is -Y.
		double BearingDegrees(const Vec3& from, const Vec3& to);

		// Bearing relative to where the player is looking. Positive means the
		// target is to the player's right. Result is in (-180, 180].
		double RelativeBearing(double bearingDeg, double playerYawDeg);

		// "N", "NE", "E", ... for a compass bearing.
		std::string CardinalName(double bearingDeg);

		// A turn instruction based on a relative bearing: "ahead", "slightly
		// right", "hard left", "behind you", ...
		std::string TurnHint(double relativeBearingDeg);

		// An arrow glyph pointing at the target, oriented to the player's view.
		std::string TurnArrow(double relativeBearingDeg);

		VerticalHint Vertical(const Vec3& from, const Vec3& to, double toleranceCm);
		std::string VerticalName(VerticalHint hint);

		// Normalize any angle into [0, 360).
		double NormalizeDegrees(double deg);

		MapCoords ToMapCoords(const Vec3& location, const MapGpsSettings& settings);
	}
}
