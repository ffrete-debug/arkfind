#pragma once

#include <string>
#include <utility>
#include <vector>

namespace ArkFind
{
	namespace Text
	{
		using Placeholders = std::vector<std::pair<std::string, std::string>>;

		// Replaces every "{key}" in `templateText` with its value. Unknown
		// placeholders are left untouched so a typo in config.json is visible in
		// game rather than silently swallowed, and replacement values are never
		// rescanned, so a value containing "{name}" cannot cause a loop.
		std::string Fill(const std::string& templateText, const Placeholders& values);

		// Fixed-point number without a trailing ".0" for whole values, used for
		// distances and coordinates in messages.
		std::string Number(double value, int decimals);
	}
}
