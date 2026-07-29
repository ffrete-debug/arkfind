#pragma once

#include <string>
#include <unordered_map>

#include "Geo.h"
#include "Session.h"

namespace ArkFind
{
	// Everything the plugin reads out of config.json.
	//
	// The struct itself is plain data with working defaults, so the rest of the
	// plugin never has to care whether a config file was present.
	struct Config
	{
		double SearchRadiusMeters = 2000.0;
		int MaxResults = 15;
		double MatchThreshold = 0.35;
		bool IncludeTamed = false;
		int MinLevel = 0;
		int MaxLevel = 0;

		int UpdateIntervalSeconds = 3;
		double ArrivalRadiusMeters = 15.0;
		double VerticalToleranceMeters = 8.0;
		bool ShowMapCoords = true;
		int TrackingTimeoutSeconds = 600;

		bool RequireAdmin = false;

		Geo::MapGpsSettings Map;

		// User visible strings, keyed by message id. Missing ids fall back to the
		// built-in English text.
		std::unordered_map<std::string, std::string> Messages;

		SearchOptions ToSearchOptions() const;
		DirectionOptions ToDirectionOptions() const;

		// Looks up a message, falling back to `fallback` when unset.
		std::string Message(const std::string& id, const std::string& fallback) const;
	};

	// Loads config.json from the plugin directory. Returns false and leaves `out`
	// at its defaults when the file is missing or malformed.
	//
	// `error` is a diagnostic, not a status: it also comes back non-empty on
	// success when a value had to be corrected (an unusable GPS scale, say), so
	// callers should log it whenever it is set.
	bool LoadConfig(const std::string& path, Config& out, std::string& error);
}
