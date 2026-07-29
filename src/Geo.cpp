#include "Geo.h"

#include <cmath>

namespace ArkFind
{
	namespace Geo
	{
		namespace
		{
			constexpr double Pi = 3.14159265358979323846;

			double ToDegrees(double radians)
			{
				return radians * (180.0 / Pi);
			}
		}

		double Distance2D(const Vec3& from, const Vec3& to)
		{
			const double dx = to.X - from.X;
			const double dy = to.Y - from.Y;
			return std::sqrt(dx * dx + dy * dy);
		}

		double Distance3D(const Vec3& from, const Vec3& to)
		{
			const double dx = to.X - from.X;
			const double dy = to.Y - from.Y;
			const double dz = to.Z - from.Z;
			return std::sqrt(dx * dx + dy * dy + dz * dz);
		}

		double CmToMeters(double cm)
		{
			return cm / CmPerMeter;
		}

		double NormalizeDegrees(double deg)
		{
			double result = std::fmod(deg, 360.0);
			if (result < 0.0)
			{
				result += 360.0;
			}
			return result;
		}

		double BearingDegrees(const Vec3& from, const Vec3& to)
		{
			const double east = to.X - from.X;
			const double north = -(to.Y - from.Y);

			if (east == 0.0 && north == 0.0)
			{
				return 0.0;
			}

			return NormalizeDegrees(ToDegrees(std::atan2(east, north)));
		}

		double RelativeBearing(double bearingDeg, double playerYawDeg)
		{
			double delta = NormalizeDegrees(bearingDeg - playerYawDeg);
			if (delta > 180.0)
			{
				delta -= 360.0;
			}
			return delta;
		}

		std::string CardinalName(double bearingDeg)
		{
			static const char* const names[] = {
				"N", "NE", "E", "SE", "S", "SW", "W", "NW"
			};

			const double normalized = NormalizeDegrees(bearingDeg);
			const int sector = static_cast<int>(std::floor((normalized + 22.5) / 45.0)) % 8;
			return names[sector];
		}

		std::string TurnHint(double relativeBearingDeg)
		{
			const double magnitude = std::fabs(relativeBearingDeg);
			const bool right = relativeBearingDeg >= 0.0;

			if (magnitude <= 12.0)
			{
				return "straight ahead";
			}
			if (magnitude <= 45.0)
			{
				return right ? "slightly right" : "slightly left";
			}
			if (magnitude <= 100.0)
			{
				return right ? "to your right" : "to your left";
			}
			if (magnitude <= 155.0)
			{
				return right ? "hard right" : "hard left";
			}
			return "behind you";
		}

		std::string TurnArrow(double relativeBearingDeg)
		{
			const double magnitude = std::fabs(relativeBearingDeg);
			const bool right = relativeBearingDeg >= 0.0;

			if (magnitude <= 22.5)
			{
				return "^";
			}
			if (magnitude <= 67.5)
			{
				return right ? "/^" : "^\\";
			}
			if (magnitude <= 112.5)
			{
				return right ? ">" : "<";
			}
			if (magnitude <= 157.5)
			{
				return right ? "\\v" : "v/";
			}
			return "v";
		}

		VerticalHint Vertical(const Vec3& from, const Vec3& to, double toleranceCm)
		{
			const double dz = to.Z - from.Z;
			if (dz > toleranceCm)
			{
				return VerticalHint::Above;
			}
			if (dz < -toleranceCm)
			{
				return VerticalHint::Below;
			}
			return VerticalHint::Level;
		}

		std::string VerticalName(VerticalHint hint)
		{
			switch (hint)
			{
			case VerticalHint::Above:
				return "above you";
			case VerticalHint::Below:
				return "below you";
			case VerticalHint::Level:
			default:
				return "";
			}
		}

		MapCoords ToMapCoords(const Vec3& location, const MapGpsSettings& settings)
		{
			MapCoords coords;
			if (settings.LatScale != 0.0)
			{
				coords.Lat = (location.Y - settings.LatOrigin) / (settings.LatScale * CmPerMeter) * 100.0;
			}
			if (settings.LonScale != 0.0)
			{
				coords.Lon = (location.X - settings.LonOrigin) / (settings.LonScale * CmPerMeter) * 100.0;
			}
			return coords;
		}
	}
}
