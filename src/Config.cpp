#include "Config.h"

#include <fstream>
#include <sstream>

// nlohmann/json ships with the ServerAPI SDK.
#include <json.hpp>

namespace ArkFind
{
	namespace
	{
		template <typename T>
		void Read(const nlohmann::json& node, const char* key, T& target)
		{
			const auto it = node.find(key);
			if (it != node.end() && !it->is_null())
			{
				target = it->get<T>();
			}
		}
	}

	SearchOptions Config::ToSearchOptions() const
	{
		SearchOptions options;
		options.RadiusCm = SearchRadiusMeters * Geo::CmPerMeter;
		options.MaxResults = MaxResults;
		options.MatchThreshold = MatchThreshold;
		options.IncludeTamed = IncludeTamed;
		options.MinLevel = MinLevel;
		options.MaxLevel = MaxLevel;
		return options;
	}

	DirectionOptions Config::ToDirectionOptions() const
	{
		DirectionOptions options;
		options.ArrivalRadiusCm = ArrivalRadiusMeters * Geo::CmPerMeter;
		options.VerticalToleranceCm = VerticalToleranceMeters * Geo::CmPerMeter;
		options.ShowMapCoords = ShowMapCoords;
		options.Gps = Map;
		return options;
	}

	std::string Config::Message(const std::string& id, const std::string& fallback) const
	{
		const auto it = Messages.find(id);
		if (it == Messages.end() || it->second.empty())
		{
			return fallback;
		}
		return it->second;
	}

	bool LoadConfig(const std::string& path, Config& out, std::string& error)
	{
		std::ifstream file(path);
		if (!file.is_open())
		{
			error = "could not open " + path;
			return false;
		}

		nlohmann::json root;
		try
		{
			file >> root;
		}
		catch (const std::exception& e)
		{
			error = std::string("invalid json: ") + e.what();
			return false;
		}

		if (!root.is_object())
		{
			error = "config root is not an object";
			return false;
		}

		Read(root, "SearchRadiusMeters", out.SearchRadiusMeters);
		Read(root, "MaxResults", out.MaxResults);
		Read(root, "MatchThreshold", out.MatchThreshold);
		Read(root, "IncludeTamed", out.IncludeTamed);
		Read(root, "MinLevel", out.MinLevel);
		Read(root, "MaxLevel", out.MaxLevel);
		Read(root, "UpdateIntervalSeconds", out.UpdateIntervalSeconds);
		Read(root, "ArrivalRadiusMeters", out.ArrivalRadiusMeters);
		Read(root, "VerticalToleranceMeters", out.VerticalToleranceMeters);
		Read(root, "ShowMapCoords", out.ShowMapCoords);
		Read(root, "TrackingTimeoutSeconds", out.TrackingTimeoutSeconds);
		Read(root, "RequireAdmin", out.RequireAdmin);

		const auto map = root.find("Map");
		if (map != root.end() && map->is_object())
		{
			Read(*map, "LatOrigin", out.Map.LatOrigin);
			Read(*map, "LonOrigin", out.Map.LonOrigin);
			Read(*map, "LatScale", out.Map.LatScale);
			Read(*map, "LonScale", out.Map.LonScale);
		}

		const auto messages = root.find("Messages");
		if (messages != root.end() && messages->is_object())
		{
			for (auto it = messages->begin(); it != messages->end(); ++it)
			{
				if (it.value().is_string())
				{
					out.Messages[it.key()] = it.value().get<std::string>();
				}
			}
		}

		// Guard against values that would break the runtime loop.
		if (out.UpdateIntervalSeconds < 1)
		{
			out.UpdateIntervalSeconds = 1;
		}
		if (out.MaxResults < 1)
		{
			out.MaxResults = 1;
		}
		if (out.SearchRadiusMeters <= 0.0)
		{
			out.SearchRadiusMeters = 2000.0;
		}
		if (out.ArrivalRadiusMeters <= 0.0)
		{
			out.ArrivalRadiusMeters = 15.0;
		}

		error.clear();
		return true;
	}
}
