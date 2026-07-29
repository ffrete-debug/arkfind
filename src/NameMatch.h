#pragma once

#include <string>
#include <vector>

namespace ArkFind
{
	namespace NameMatch
	{
		// Lowercases, strips punctuation and collapses whitespace so that
		// "Alpha  T-Rex!" and "alpha t rex" compare equal.
		std::string Normalize(const std::string& text);

		// Pulls the mod tag out of a blueprint path.
		//
		//   /Game/PrimalEarth/Dinos/Rex/Rex_Character_BP.Rex_Character_BP_C  -> ""
		//   /Game/Mods/AE/Dinos/Wyvern/Void_Character_BP.Void_Character_BP_C -> "AE"
		std::string ModTagFromBlueprint(const std::string& blueprintPath);

		// Trailing class tag of a blueprint path, without the "_C" suffix.
		std::string ClassNameFromBlueprint(const std::string& blueprintPath);

		// 0.0 (no match) .. 1.0 (exact). Exact beats prefix beats word-prefix
		// beats substring beats fuzzy subsequence, so searching "rex" ranks "Rex"
		// above "Rex Bone Costume" above "Tyrannosaurus".
		//
		// The tiers occupy fixed bands, which is what MatchThreshold selects
		// between:
		//   exact                 1.0
		//   prefix                0.85 - 0.95
		//   word prefix           0.70 - 0.80
		//   substring             0.55 - 0.65
		//   fuzzy subsequence     0.35 - 0.45
		// Within a band, a shorter candidate scores higher.
		//
		// Matching is one-directional: the query must be contained in the
		// candidate, in order. "rex" matches "Alpha Rex", "rex alpha" does not.
		double Score(const std::string& query, const std::string& candidate);

		// True when the query should be considered a hit at all. A score of 0.0 is
		// never a match, even when `threshold` is 0.0.
		bool Matches(const std::string& query, const std::string& candidate, double threshold = 0.35);

		// Best score across several candidate strings (display name, class name,
		// blueprint path), so mod dinos are findable by any of their names.
		double BestScore(const std::string& query, const std::vector<std::string>& candidates);
	}
}
