#include "test_framework.h"

#include "Text.h"

namespace Text = ArkFind::Text;

TEST_CASE(Text_Fill_ReplacesKnownPlaceholders)
{
	const Text::Placeholders values = { {"name", "Rex"}, {"level", "145"} };
	CHECK_STR_EQ(Text::Fill("Tracking {name} (lvl {level})", values), "Tracking Rex (lvl 145)");
}

TEST_CASE(Text_Fill_RepeatsAPlaceholder)
{
	const Text::Placeholders values = { {"n", "x"} };
	CHECK_STR_EQ(Text::Fill("{n}{n}{n}", values), "xxx");
}

TEST_CASE(Text_Fill_LeavesUnknownPlaceholderVisible)
{
	const Text::Placeholders values = { {"name", "Rex"} };
	CHECK_STR_EQ(Text::Fill("{name} at {nope}", values), "Rex at {nope}");
}

TEST_CASE(Text_Fill_DoesNotRescanReplacementValues)
{
	// A value that itself looks like a placeholder must not be expanded again.
	const Text::Placeholders values = { {"a", "{a}"} };
	CHECK_STR_EQ(Text::Fill("{a}", values), "{a}");
}

TEST_CASE(Text_Fill_HandlesUnterminatedBrace)
{
	const Text::Placeholders values = { {"name", "Rex"} };
	CHECK_STR_EQ(Text::Fill("hello {name", values), "hello {name");
	CHECK_STR_EQ(Text::Fill("{", values), "{");
}

TEST_CASE(Text_Fill_HandlesEmptyInputs)
{
	CHECK_STR_EQ(Text::Fill("", {}), "");
	CHECK_STR_EQ(Text::Fill("plain text", {}), "plain text");
	CHECK_STR_EQ(Text::Fill("{}", { {"", "empty"} }), "empty");
}

TEST_CASE(Text_Fill_FirstMatchWins)
{
	const Text::Placeholders values = { {"k", "first"}, {"k", "second"} };
	CHECK_STR_EQ(Text::Fill("{k}", values), "first");
}

TEST_CASE(Text_Number_FormatsWithRequestedPrecision)
{
	CHECK_STR_EQ(Text::Number(41.2549, 1), "41.3");
	CHECK_STR_EQ(Text::Number(1234.0, 0), "1234");
	CHECK_STR_EQ(Text::Number(-7.5, 2), "-7.50");
}

TEST_CASE(Text_Number_ClampsDecimals)
{
	CHECK_STR_EQ(Text::Number(1.5, -3), "2");
	CHECK(!Text::Number(1.0, 20).empty());
}
