#pragma once

// Tiny header-only test harness for the ArkFind pure modules.
//
// No external dependencies: every test translation unit includes this header,
// declares its cases with TEST_CASE(SomeIdentifier), and test_main.cpp calls
// RunAllTests(). Cases self-register through a static Registrar object, so the
// order of registration is the order of definition inside each TU.

#include <cmath>
#include <cstdio>
#include <string>
#include <type_traits>
#include <vector>

namespace TestFramework
{
	struct Failure
	{
		const char* Test = "";
		const char* File = "";
		int Line = 0;
		std::string Message;
	};

	using TestFn = void (*)();

	struct TestCase
	{
		const char* Name = "";
		TestFn Fn = nullptr;
	};

	inline std::vector<TestCase>& Registry()
	{
		static std::vector<TestCase> registry;
		return registry;
	}

	inline std::vector<Failure>& CurrentFailures()
	{
		static std::vector<Failure> failures;
		return failures;
	}

	struct Registrar
	{
		Registrar(const char* name, TestFn fn)
		{
			Registry().push_back(TestCase{ name, fn });
		}
	};

	inline void Report(const char* file, int line, const std::string& message)
	{
		Failure failure;
		failure.File = file;
		failure.Line = line;
		failure.Message = message;
		CurrentFailures().push_back(failure);
	}

	// ---- value formatting for failure messages ------------------------------

	inline std::string ToText(const std::string& value)
	{
		return "\"" + value + "\"";
	}

	inline std::string ToText(const char* value)
	{
		return std::string("\"") + (value != nullptr ? value : "(null)") + "\"";
	}

	inline std::string ToText(bool value)
	{
		return value ? "true" : "false";
	}

	inline std::string ToText(double value)
	{
		char buffer[64];
		std::snprintf(buffer, sizeof(buffer), "%.10g", value);
		return buffer;
	}

	inline std::string ToText(float value)
	{
		return ToText(static_cast<double>(value));
	}

	inline std::string ToText(int value) { return std::to_string(value); }
	inline std::string ToText(long value) { return std::to_string(value); }
	inline std::string ToText(long long value) { return std::to_string(value); }
	inline std::string ToText(unsigned int value) { return std::to_string(value); }
	inline std::string ToText(unsigned long value) { return std::to_string(value); }
	inline std::string ToText(unsigned long long value) { return std::to_string(value); }
	inline std::string ToText(char value) { return std::string(1, value); }

	template <typename T>
	inline typename std::enable_if<std::is_enum<T>::value, std::string>::type ToText(T value)
	{
		return "enum(" + std::to_string(static_cast<long long>(value)) + ")";
	}

	// ---- runner -------------------------------------------------------------

	inline int RunAllTests()
	{
		std::vector<Failure> allFailures;
		int passed = 0;
		int total = 0;

		for (const TestCase& testCase : Registry())
		{
			++total;
			CurrentFailures().clear();
			testCase.Fn();

			if (CurrentFailures().empty())
			{
				++passed;
			}
			else
			{
				for (Failure failure : CurrentFailures())
				{
					failure.Test = testCase.Name;
					allFailures.push_back(failure);
				}
			}
		}

		if (!allFailures.empty())
		{
			std::printf("FAILURES:\n");
			for (const Failure& failure : allFailures)
			{
				std::printf("  %s:%d  [%s]\n    %s\n",
					failure.File, failure.Line, failure.Test, failure.Message.c_str());
			}
			std::printf("\n");
		}

		std::printf("%d/%d passed\n", passed, total);
		return total - passed;
	}
}

#define TEST_CASE(name)                                                       \
	static void name();                                                       \
	static ::TestFramework::Registrar arkfind_test_registrar_##name(#name, &name); \
	static void name()

#define CHECK(expr)                                                           \
	do                                                                        \
	{                                                                         \
		if (!(expr))                                                          \
		{                                                                     \
			::TestFramework::Report(__FILE__, __LINE__,                       \
				std::string("CHECK(") + #expr + ") is false");                \
		}                                                                     \
	} while (false)

#define CHECK_EQ(a, b)                                                        \
	do                                                                        \
	{                                                                         \
		const auto arkfind_check_a = (a);                                      \
		const auto arkfind_check_b = (b);                                      \
		if (!(arkfind_check_a == arkfind_check_b))                             \
		{                                                                     \
			::TestFramework::Report(__FILE__, __LINE__,                       \
				std::string("CHECK_EQ(") + #a + ", " + #b + ")\n      got:      " \
				+ ::TestFramework::ToText(arkfind_check_a)                     \
				+ "\n      expected: " + ::TestFramework::ToText(arkfind_check_b)); \
		}                                                                     \
	} while (false)

#define CHECK_NEAR(a, b, eps)                                                 \
	do                                                                        \
	{                                                                         \
		const double arkfind_near_a = static_cast<double>(a);                  \
		const double arkfind_near_b = static_cast<double>(b);                  \
		const double arkfind_near_eps = static_cast<double>(eps);              \
		if (!(std::fabs(arkfind_near_a - arkfind_near_b) <= arkfind_near_eps)) \
		{                                                                     \
			::TestFramework::Report(__FILE__, __LINE__,                       \
				std::string("CHECK_NEAR(") + #a + ", " + #b + ", " + #eps + ")\n      got:      " \
				+ ::TestFramework::ToText(arkfind_near_a)                      \
				+ "\n      expected: " + ::TestFramework::ToText(arkfind_near_b) \
				+ " +/- " + ::TestFramework::ToText(arkfind_near_eps));        \
		}                                                                     \
	} while (false)

#define CHECK_STR_EQ(a, b)                                                    \
	do                                                                        \
	{                                                                         \
		const std::string arkfind_str_a = (a);                                 \
		const std::string arkfind_str_b = (b);                                 \
		if (arkfind_str_a != arkfind_str_b)                                    \
		{                                                                     \
			::TestFramework::Report(__FILE__, __LINE__,                       \
				std::string("CHECK_STR_EQ(") + #a + ", " + #b + ")\n      got:      \"" \
				+ arkfind_str_a + "\"\n      expected: \"" + arkfind_str_b + "\""); \
		}                                                                     \
	} while (false)
