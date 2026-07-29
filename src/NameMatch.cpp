#include "NameMatch.h"

#include <algorithm>
#include <cctype>

namespace ArkFind
{
	namespace NameMatch
	{
		namespace
		{
			bool IsWordChar(unsigned char c)
			{
				return std::isalnum(c) != 0;
			}

			// True when `needle` appears in `haystack` in order, but not
			// necessarily contiguously ("rx" matches "rex").
			bool IsSubsequence(const std::string& needle, const std::string& haystack)
			{
				size_t n = 0;
				for (size_t h = 0; h < haystack.size() && n < needle.size(); ++h)
				{
					if (haystack[h] == needle[n])
					{
						++n;
					}
				}
				return n == needle.size();
			}

			bool StartsWithWord(const std::string& haystack, const std::string& needle)
			{
				size_t pos = 0;
				while (pos != std::string::npos)
				{
					if (haystack.compare(pos, needle.size(), needle) == 0)
					{
						return true;
					}
					pos = haystack.find(' ', pos);
					if (pos != std::string::npos)
					{
						++pos;
					}
				}
				return false;
			}
		}

		std::string Normalize(const std::string& text)
		{
			std::string out;
			out.reserve(text.size());

			bool pendingSpace = false;
			for (const char raw : text)
			{
				const unsigned char c = static_cast<unsigned char>(raw);
				if (IsWordChar(c))
				{
					if (pendingSpace && !out.empty())
					{
						out.push_back(' ');
					}
					pendingSpace = false;
					out.push_back(static_cast<char>(std::tolower(c)));
				}
				else
				{
					pendingSpace = true;
				}
			}

			return out;
		}

		std::string ModTagFromBlueprint(const std::string& blueprintPath)
		{
			const std::string marker = "/Mods/";
			const size_t start = blueprintPath.find(marker);
			if (start == std::string::npos)
			{
				return "";
			}

			const size_t tagStart = start + marker.size();
			const size_t tagEnd = blueprintPath.find('/', tagStart);
			if (tagEnd == std::string::npos)
			{
				return blueprintPath.substr(tagStart);
			}
			return blueprintPath.substr(tagStart, tagEnd - tagStart);
		}

		std::string ClassNameFromBlueprint(const std::string& blueprintPath)
		{
			if (blueprintPath.empty())
			{
				return "";
			}

			// Blueprint paths look like "/Game/.../Rex_Character_BP.Rex_Character_BP_C".
			size_t start = blueprintPath.find_last_of("./");
			start = (start == std::string::npos) ? 0 : start + 1;

			std::string name = blueprintPath.substr(start);
			if (name.size() > 2 && name.compare(name.size() - 2, 2, "_C") == 0)
			{
				name.erase(name.size() - 2);
			}
			return name;
		}

		double Score(const std::string& query, const std::string& candidate)
		{
			const std::string q = Normalize(query);
			const std::string c = Normalize(candidate);

			if (q.empty() || c.empty())
			{
				return 0.0;
			}

			if (q == c)
			{
				return 1.0;
			}

			// Shorter candidates win ties: "Rex" should outrank "Rex Saddle".
			const double lengthBonus = static_cast<double>(q.size()) / static_cast<double>(c.size());

			if (c.compare(0, q.size(), q) == 0)
			{
				return 0.85 + 0.10 * lengthBonus;
			}

			if (StartsWithWord(c, q))
			{
				return 0.70 + 0.10 * lengthBonus;
			}

			if (c.find(q) != std::string::npos)
			{
				return 0.55 + 0.10 * lengthBonus;
			}

			if (IsSubsequence(q, c))
			{
				return 0.35 + 0.10 * lengthBonus;
			}

			return 0.0;
		}

		bool Matches(const std::string& query, const std::string& candidate, double threshold)
		{
			return Score(query, candidate) >= threshold;
		}

		double BestScore(const std::string& query, const std::vector<std::string>& candidates)
		{
			double best = 0.0;
			for (const std::string& candidate : candidates)
			{
				best = std::max(best, Score(query, candidate));
			}
			return best;
		}
	}
}
