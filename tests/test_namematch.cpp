#include "test_framework.h"

#include "NameMatch.h"

namespace NM = ArkFind::NameMatch;

// ----------------------------------------------------------------- Normalize

TEST_CASE(NameMatch_Normalize_LowercasesAndStripsPunctuation)
{
	CHECK_STR_EQ(NM::Normalize("Rex"), "rex");
	CHECK_STR_EQ(NM::Normalize("REX"), "rex");
	CHECK_STR_EQ(NM::Normalize("ReX"), "rex");
	CHECK_STR_EQ(NM::Normalize("Alpha  T-Rex!"), "alpha t rex");
	CHECK_STR_EQ(NM::Normalize("alpha t rex"), "alpha t rex");
	// The whole point: those two compare equal.
	CHECK_STR_EQ(NM::Normalize("Alpha  T-Rex!"), NM::Normalize("alpha t rex"));
}

TEST_CASE(NameMatch_Normalize_CollapsesWhitespaceAndTrims)
{
	CHECK_STR_EQ(NM::Normalize("  Rex  "), "rex");
	CHECK_STR_EQ(NM::Normalize("\tRex\n"), "rex");
	CHECK_STR_EQ(NM::Normalize("Ice     Wyvern"), "ice wyvern");
	CHECK_STR_EQ(NM::Normalize("Ice\t\tWyvern  "), "ice wyvern");
	CHECK_STR_EQ(NM::Normalize("---Rex---"), "rex");
	// A run of separators collapses to exactly one space.
	CHECK_STR_EQ(NM::Normalize("a - _ . b"), "a b");
}

TEST_CASE(NameMatch_Normalize_KeepsDigitsAndDropsUnderscores)
{
	CHECK_STR_EQ(NM::Normalize("Rex_Character_BP"), "rex character bp");
	CHECK_STR_EQ(NM::Normalize("R-2000"), "r 2000");
	CHECK_STR_EQ(NM::Normalize("Level150"), "level150");
	CHECK_STR_EQ(NM::Normalize("/Game/Mods/AE/Void_Character_BP.Void_Character_BP_C"),
		"game mods ae void character bp void character bp c");
}

TEST_CASE(NameMatch_Normalize_EmptyAndPunctuationOnly)
{
	CHECK_STR_EQ(NM::Normalize(""), "");
	CHECK_STR_EQ(NM::Normalize("   "), "");
	CHECK_STR_EQ(NM::Normalize("!!!"), "");
	CHECK_STR_EQ(NM::Normalize("-_-"), "");
}

// --------------------------------------------------------- ModTagFromBlueprint

TEST_CASE(NameMatch_ModTagFromBlueprint_VanillaHasNoTag)
{
	CHECK_STR_EQ(NM::ModTagFromBlueprint(
		"/Game/PrimalEarth/Dinos/Rex/Rex_Character_BP.Rex_Character_BP_C"), "");
	CHECK_STR_EQ(NM::ModTagFromBlueprint(
		"/Game/Aberration/Dinos/RockDrake/RockDrake_Character_BP.RockDrake_Character_BP_C"), "");
	CHECK_STR_EQ(NM::ModTagFromBlueprint(""), "");
	CHECK_STR_EQ(NM::ModTagFromBlueprint("Rex_Character_BP_C"), "");
	// "Mods" must be a path segment, not just a substring of a name.
	CHECK_STR_EQ(NM::ModTagFromBlueprint("/Game/PrimalEarth/Modserver_Character_BP"), "");
}

TEST_CASE(NameMatch_ModTagFromBlueprint_ModPaths)
{
	CHECK_STR_EQ(NM::ModTagFromBlueprint(
		"/Game/Mods/AE/Dinos/Wyvern/Void_Character_BP.Void_Character_BP_C"), "AE");
	CHECK_STR_EQ(NM::ModTagFromBlueprint(
		"/Game/Mods/1404697612/Dinos/Foo.Foo_C"), "1404697612");
	// Tag is the last segment, so no trailing slash still works.
	CHECK_STR_EQ(NM::ModTagFromBlueprint("/Game/Mods/AE"), "AE");
	// Quoted / wrapped blueprint references still parse.
	CHECK_STR_EQ(NM::ModTagFromBlueprint(
		"Blueprint'/Game/Mods/Ext/Dinos/Foo.Foo_C'"), "Ext");
	// Case is preserved verbatim.
	CHECK_STR_EQ(NM::ModTagFromBlueprint("/Game/Mods/MyCoolMod/X.X_C"), "MyCoolMod");
	// Empty segment right after the marker.
	CHECK_STR_EQ(NM::ModTagFromBlueprint("/Game/Mods//X.X_C"), "");
}

// ------------------------------------------------------ ClassNameFromBlueprint

TEST_CASE(NameMatch_ClassNameFromBlueprint_StripsPathAndUnderscoreC)
{
	CHECK_STR_EQ(NM::ClassNameFromBlueprint(
		"/Game/PrimalEarth/Dinos/Rex/Rex_Character_BP.Rex_Character_BP_C"), "Rex_Character_BP");
	CHECK_STR_EQ(NM::ClassNameFromBlueprint(
		"/Game/Mods/AE/Dinos/Wyvern/Void_Character_BP.Void_Character_BP_C"), "Void_Character_BP");
	// Already just a class tag.
	CHECK_STR_EQ(NM::ClassNameFromBlueprint("Rex_Character_BP_C"), "Rex_Character_BP");
	// Path with no dot: last '/' wins.
	CHECK_STR_EQ(NM::ClassNameFromBlueprint("/Game/Dinos/Rex_Character_BP"), "Rex_Character_BP");
}

TEST_CASE(NameMatch_ClassNameFromBlueprint_EdgeCases)
{
	CHECK_STR_EQ(NM::ClassNameFromBlueprint(""), "");
	// No "_C" suffix -> untouched.
	CHECK_STR_EQ(NM::ClassNameFromBlueprint("Rex"), "Rex");
	CHECK_STR_EQ(NM::ClassNameFromBlueprint("Rex_Character_BP"), "Rex_Character_BP");
	// "_C" alone is too short to be a suffix on top of a name.
	CHECK_STR_EQ(NM::ClassNameFromBlueprint("_C"), "_C");
	CHECK_STR_EQ(NM::ClassNameFromBlueprint("A_C"), "A");
	// Trailing separator leaves nothing behind it.
	CHECK_STR_EQ(NM::ClassNameFromBlueprint("/Game/Dinos/"), "");
	// Lowercase "_c" is not the suffix.
	CHECK_STR_EQ(NM::ClassNameFromBlueprint("Rex_c"), "Rex_c");
}

// --------------------------------------------------------------------- Score

TEST_CASE(NameMatch_Score_ExactIsOne)
{
	CHECK_NEAR(NM::Score("Rex", "Rex"), 1.0, 1e-12);
	// Normalization applies to both sides.
	CHECK_NEAR(NM::Score("rex", "REX"), 1.0, 1e-12);
	CHECK_NEAR(NM::Score("  ice-wyvern ", "Ice Wyvern"), 1.0, 1e-12);
	CHECK_NEAR(NM::Score("rex character bp", "Rex_Character_BP"), 1.0, 1e-12);
}

TEST_CASE(NameMatch_Score_EmptyInputsScoreZero)
{
	CHECK_NEAR(NM::Score("", "Rex"), 0.0, 1e-12);
	CHECK_NEAR(NM::Score("Rex", ""), 0.0, 1e-12);
	CHECK_NEAR(NM::Score("", ""), 0.0, 1e-12);
	// Punctuation-only normalizes to empty.
	CHECK_NEAR(NM::Score("!!!", "Rex"), 0.0, 1e-12);
	CHECK_NEAR(NM::Score("Rex", "---"), 0.0, 1e-12);
}

TEST_CASE(NameMatch_Score_NoMatchIsZero)
{
	CHECK_NEAR(NM::Score("zzz", "Rex"), 0.0, 1e-12);
	CHECK_NEAR(NM::Score("dodo", "Rex"), 0.0, 1e-12);
	// Query longer than the candidate can never match.
	CHECK_NEAR(NM::Score("rexosaurus", "Rex"), 0.0, 1e-12);
}

TEST_CASE(NameMatch_Score_BandValues)
{
	// exact = 1.0
	CHECK_NEAR(NM::Score("rex", "Rex"), 1.0, 1e-12);
	// prefix = 0.85 + 0.10 * (len(q)/len(c))
	CHECK_NEAR(NM::Score("rex", "Rex Saddle"), 0.85 + 0.10 * (3.0 / 10.0), 1e-12);
	// word-prefix = 0.70 + 0.10 * ratio
	CHECK_NEAR(NM::Score("rex", "Ice Rex"), 0.70 + 0.10 * (3.0 / 7.0), 1e-12);
	// substring = 0.55 + 0.10 * ratio
	CHECK_NEAR(NM::Score("rex", "Trexosaurus"), 0.55 + 0.10 * (3.0 / 11.0), 1e-12);
	// subsequence = 0.35 + 0.10 * ratio
	CHECK_NEAR(NM::Score("rex", "Reaper Xenomorph"), 0.35 + 0.10 * (3.0 / 16.0), 1e-12);
}

TEST_CASE(NameMatch_Score_BandOrdering)
{
	const double exact = NM::Score("rex", "Rex");
	const double prefix = NM::Score("rex", "Rex Saddle");
	const double wordPrefix = NM::Score("rex", "Ice Rex");
	const double substring = NM::Score("rex", "Trexosaurus");
	const double subsequence = NM::Score("rex", "Reaper Xenomorph");
	const double none = NM::Score("rex", "Dodo");

	CHECK(exact > prefix);
	CHECK(prefix > wordPrefix);
	CHECK(wordPrefix > substring);
	CHECK(substring > subsequence);
	CHECK(subsequence > none);
	CHECK_NEAR(none, 0.0, 1e-12);

	// Bands never overlap, whatever the length bonus does.
	CHECK(prefix >= 0.85 && prefix <= 0.95);
	CHECK(wordPrefix >= 0.70 && wordPrefix <= 0.80);
	CHECK(substring >= 0.55 && substring <= 0.65);
	CHECK(subsequence >= 0.35 && subsequence <= 0.45);
}

TEST_CASE(NameMatch_Score_ShorterCandidateWinsTies)
{
	// Same band (prefix), so only length decides: "Rex" beats "Rexy" beats the
	// long costume name.
	const double shortest = NM::Score("rex", "Rexy");
	const double middle = NM::Score("rex", "Rex Saddle");
	const double longest = NM::Score("rex", "Rex Bone Costume");

	CHECK(shortest > middle);
	CHECK(middle > longest);
	CHECK_NEAR(shortest, 0.85 + 0.10 * (3.0 / 4.0), 1e-12);
	CHECK_NEAR(longest, 0.85 + 0.10 * (3.0 / 16.0), 1e-12);

	// And the exact name still beats every prefix variant.
	CHECK(NM::Score("rex", "Rex") > shortest);
}

TEST_CASE(NameMatch_Score_WordPrefixNeedsAWordBoundary)
{
	// "bone" starts the second word of "Rex Bone Costume".
	CHECK_NEAR(NM::Score("bone", "Rex Bone Costume"), 0.70 + 0.10 * (4.0 / 16.0), 1e-12);
	// "os" is inside "costume" but starts no word -> substring band only.
	CHECK_NEAR(NM::Score("os", "Rex Bone Costume"), 0.55 + 0.10 * (2.0 / 16.0), 1e-12);
	// Underscores normalize to spaces, so class tags get word-prefix treatment.
	CHECK_NEAR(NM::Score("character", "Rex_Character_BP"), 0.70 + 0.10 * (9.0 / 16.0), 1e-12);
}

TEST_CASE(NameMatch_Score_FuzzySubsequence)
{
	// "rx" hits r-e-x in order without being contiguous.
	CHECK_NEAR(NM::Score("rx", "Rex"), 0.35 + 0.10 * (2.0 / 3.0), 1e-12);
	// Out of order fails entirely.
	CHECK_NEAR(NM::Score("xr", "Rex"), 0.0, 1e-12);
	// Missing letter fails.
	CHECK_NEAR(NM::Score("rez", "Rex"), 0.0, 1e-12);
}

// ------------------------------------------------------------------- Matches

TEST_CASE(NameMatch_Matches_DefaultThreshold)
{
	// Default threshold 0.35 admits the weakest (subsequence) band.
	CHECK(NM::Matches("rex", "Rex"));
	CHECK(NM::Matches("rex", "Rex Saddle"));
	CHECK(NM::Matches("rex", "Ice Rex"));
	CHECK(NM::Matches("rex", "Trexosaurus"));
	CHECK(NM::Matches("rx", "Rex"));
	CHECK(!NM::Matches("zzz", "Rex"));
	CHECK(!NM::Matches("", "Rex"));
	CHECK(!NM::Matches("rex", ""));
}

TEST_CASE(NameMatch_Matches_ExplicitThreshold)
{
	// Threshold comparison is >=.
	CHECK(NM::Matches("rex", "Rex", 1.0));
	CHECK(!NM::Matches("rex", "Rexy", 1.0));
	CHECK(NM::Matches("rex", "Rexy", 0.85));
	CHECK(!NM::Matches("rex", "Trexosaurus", 0.70));
	CHECK(NM::Matches("rex", "Trexosaurus", 0.55));
	// A zero threshold still rejects nothing-in-common pairs only via the score.
	CHECK(NM::Matches("zzz", "Rex", 0.0));
}

// ----------------------------------------------------------------- BestScore

TEST_CASE(NameMatch_BestScore_PicksTheStrongestCandidate)
{
	const std::vector<std::string> candidates = {
		"Rex Bone Costume",
		"Trexosaurus",
		"Rex"
	};
	CHECK_NEAR(NM::BestScore("rex", candidates), 1.0, 1e-12);

	const std::vector<std::string> weaker = {
		"Rex Bone Costume",
		"Trexosaurus"
	};
	CHECK_NEAR(NM::BestScore("rex", weaker), NM::Score("rex", "Rex Bone Costume"), 1e-12);
}

TEST_CASE(NameMatch_BestScore_EmptyAndUnmatched)
{
	CHECK_NEAR(NM::BestScore("rex", {}), 0.0, 1e-12);
	CHECK_NEAR(NM::BestScore("rex", { "", "", "" }), 0.0, 1e-12);
	CHECK_NEAR(NM::BestScore("zzz", { "Rex", "Dodo" }), 0.0, 1e-12);
	CHECK_NEAR(NM::BestScore("", { "Rex" }), 0.0, 1e-12);
}

TEST_CASE(NameMatch_BestScore_FindsModDinoByAnyOfItsNames)
{
	const std::string blueprint =
		"/Game/Mods/AE/Dinos/Wyvern/Void_Character_BP.Void_Character_BP_C";
	const std::vector<std::string> candidates = {
		"Voidwyrm",
		NM::ClassNameFromBlueprint(blueprint),
		blueprint,
		NM::ModTagFromBlueprint(blueprint) + " Voidwyrm"
	};

	CHECK_STR_EQ(candidates[1], "Void_Character_BP");
	CHECK_STR_EQ(candidates[3], "AE Voidwyrm");

	CHECK(NM::BestScore("voidwyrm", candidates) >= 1.0);       // display name, exact
	CHECK(NM::BestScore("void", candidates) >= 0.85);          // class name prefix
	CHECK(NM::BestScore("ae voidwyrm", candidates) >= 1.0);    // mod tag + name, exact
	CHECK(NM::BestScore("wyvern", candidates) >= 0.35);        // blueprint folder
	CHECK_NEAR(NM::BestScore("stegosaurus", candidates), 0.0, 1e-12);
}
