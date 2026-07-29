#include "Session.h"

#include "NameMatch.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace ArkFind
{
	namespace
	{
		std::string FormatNumber(double value, int decimals)
		{
			char buffer[64];
			std::snprintf(buffer, sizeof(buffer), "%.*f", decimals, value);
			return buffer;
		}

		std::string FormatDistance(double distanceCm)
		{
			const double meters = Geo::CmToMeters(distanceCm);
			if (meters < 1000.0)
			{
				return FormatNumber(meters, 0) + "m";
			}
			return FormatNumber(meters / 1000.0, 2) + "km";
		}
	}

	std::vector<SearchHit> RankResults(const std::vector<DinoInfo>& dinos,
		const Vec3& playerLocation,
		const std::string& query,
		const SearchOptions& options)
	{
		std::vector<SearchHit> hits;
		hits.reserve(dinos.size());

		const bool matchAll = NameMatch::Normalize(query).empty();

		for (const DinoInfo& dino : dinos)
		{
			if (!dino.IsAlive)
			{
				continue;
			}
			if (dino.IsTamed && !options.IncludeTamed)
			{
				continue;
			}
			if (options.MinLevel > 0 && dino.Level < options.MinLevel)
			{
				continue;
			}
			if (options.MaxLevel > 0 && dino.Level > options.MaxLevel)
			{
				continue;
			}

			const double distance = Geo::Distance3D(playerLocation, dino.Location);
			if (options.RadiusCm > 0.0 && distance > options.RadiusCm)
			{
				continue;
			}

			double score = 1.0;
			if (!matchAll)
			{
				score = NameMatch::BestScore(query, {
					dino.DisplayName,
					dino.ClassName,
					dino.BlueprintPath,
					dino.ModTag.empty() ? dino.DisplayName : (dino.ModTag + " " + dino.DisplayName)
				});

				if (score < options.MatchThreshold)
				{
					continue;
				}
			}

			SearchHit hit;
			hit.Dino = dino;
			hit.Dino.DistanceCm = distance;
			hit.MatchScore = score;
			hits.push_back(hit);
		}

		// Best name match first, then closest. A player searching "rex" wants the
		// nearest Rex, not the nearest Rexy-something.
		std::stable_sort(hits.begin(), hits.end(), [](const SearchHit& a, const SearchHit& b)
		{
			if (std::fabs(a.MatchScore - b.MatchScore) > 0.01)
			{
				return a.MatchScore > b.MatchScore;
			}
			return a.Dino.DistanceCm < b.Dino.DistanceCm;
		});

		if (options.MaxResults > 0 && hits.size() > static_cast<size_t>(options.MaxResults))
		{
			hits.resize(static_cast<size_t>(options.MaxResults));
		}

		return hits;
	}

	std::string FormatHitLine(int index, const SearchHit& hit)
	{
		std::string line = std::to_string(index) + ") " + hit.Dino.DisplayName;

		if (!hit.Dino.ModTag.empty())
		{
			line += " [" + hit.Dino.ModTag + "]";
		}

		line += " lvl " + std::to_string(hit.Dino.Level);
		line += hit.Dino.IsFemale ? " F" : " M";

		if (hit.Dino.IsTamed)
		{
			line += " (tamed)";
		}

		line += " - " + FormatDistance(hit.Dino.DistanceCm);
		return line;
	}

	std::string FormatDirection(const TrackedTarget& target,
		const Vec3& playerLocation,
		double playerYawDeg,
		const DirectionOptions& options)
	{
		const double distance = Geo::Distance3D(playerLocation, target.LastKnownLocation);

		if (distance <= options.ArrivalRadiusCm)
		{
			return "FOUND " + target.DisplayName + " - it is right here (" + FormatDistance(distance) + ").";
		}

		const double bearing = Geo::BearingDegrees(playerLocation, target.LastKnownLocation);
		const double relative = Geo::RelativeBearing(bearing, playerYawDeg);

		std::string line = Geo::TurnArrow(relative) + " " + target.DisplayName;
		line += " - " + FormatDistance(distance);
		line += " " + Geo::TurnHint(relative);
		line += " (" + Geo::CardinalName(bearing) + ")";

		const std::string vertical = Geo::VerticalName(
			Geo::Vertical(playerLocation, target.LastKnownLocation, options.VerticalToleranceCm));
		if (!vertical.empty())
		{
			line += ", " + vertical;
		}

		if (options.ShowMapCoords)
		{
			const Geo::MapCoords coords = Geo::ToMapCoords(target.LastKnownLocation, options.Gps);
			line += " [lat " + FormatNumber(coords.Lat, 1) + " lon " + FormatNumber(coords.Lon, 1) + "]";
		}

		return line;
	}

	bool HasArrived(const TrackedTarget& target, const Vec3& playerLocation, double arrivalRadiusCm)
	{
		return Geo::Distance3D(playerLocation, target.LastKnownLocation) <= arrivalRadiusCm;
	}

	int ParseSelection(const std::string& argument, size_t resultCount)
	{
		std::string digits;
		for (const char c : argument)
		{
			if (c >= '0' && c <= '9')
			{
				digits.push_back(c);
			}
			else if (!digits.empty())
			{
				break;
			}
		}

		if (digits.empty() || digits.size() > 4)
		{
			return -1;
		}

		const int oneBased = std::atoi(digits.c_str());
		if (oneBased < 1 || static_cast<size_t>(oneBased) > resultCount)
		{
			return -1;
		}

		return oneBased - 1;
	}
}
