cmake_minimum_required(VERSION 3.15)

project(ArkFind CXX)

# ---------------------------------------------------------------------------
# Options
#
# The pure-logic tests build and run anywhere (they only touch the standard
# library), so they are on by default and they are the real gate for CI.
#
# The plugin DLL needs Windows plus the ServerAPI / ArkApi SDK headers, so it is
# opt-in.
# ---------------------------------------------------------------------------
option(ARKFIND_BUILD_TESTS "Build the pure-logic unit tests" ON)
option(ARKFIND_BUILD_PLUGIN "Build the ARK server plugin DLL" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Warnings, shared by every target we define.
add_library(arkfind_warnings INTERFACE)
if(MSVC)
	target_compile_options(arkfind_warnings INTERFACE /W4)
else()
	target_compile_options(arkfind_warnings INTERFACE -Wall -Wextra)
endif()

# The portable, dependency-free half of the plugin. Explicit list on purpose:
# a stray or half-written file in src/ must never silently join the build.
set(ARKFIND_PURE_SOURCES
	"${CMAKE_CURRENT_SOURCE_DIR}/src/Geo.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/NameMatch.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/Session.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/Text.cpp"
)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(ARKFIND_BUILD_TESTS)
	# GLOB is deliberate here and only here: the individual test translation
	# units are authored in parallel with this file, so their names are not
	# known up front. Re-run CMake (or rely on CONFIGURE_DEPENDS) after adding
	# a new test file.
	file(GLOB ARKFIND_TEST_SOURCES CONFIGURE_DEPENDS
		"${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")

	if(NOT ARKFIND_TEST_SOURCES)
		message(FATAL_ERROR
			"ARKFIND_BUILD_TESTS is ON but no test sources were found in "
			"'${CMAKE_CURRENT_SOURCE_DIR}/tests'. Add at least one .cpp there, "
			"or configure with -DARKFIND_BUILD_TESTS=OFF.")
	endif()

	enable_testing()

	add_executable(arkfind_tests ${ARKFIND_TEST_SOURCES} ${ARKFIND_PURE_SOURCES})
	target_include_directories(arkfind_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
	target_link_libraries(arkfind_tests PRIVATE arkfind_warnings)

	# Release configurations define NDEBUG, which would turn every assert() in
	# the tests into a no-op and make the suite pass vacuously. Undefine it for
	# this target so assert-based tests keep their teeth in any configuration.
	if(MSVC)
		target_compile_options(arkfind_tests PRIVATE /UNDEBUG)
	else()
		target_compile_options(arkfind_tests PRIVATE -UNDEBUG)
	endif()

	add_test(NAME arkfind_tests COMMAND arkfind_tests)
endif()

# ---------------------------------------------------------------------------
# Plugin DLL
# ---------------------------------------------------------------------------
if(ARKFIND_BUILD_PLUGIN)
	if(NOT WIN32)
		message(FATAL_ERROR
			"ARKFIND_BUILD_PLUGIN is only supported on Windows: the ARK "
			"dedicated server loads a Win64 DLL and the ServerAPI SDK is "
			"Windows-only. Use -DARKFIND_BUILD_PLUGIN=OFF on this platform.")
	endif()

	set(ARKAPI_SDK_DIR "" CACHE PATH
		"Path to the ServerAPI / ArkApi SDK public include directory")

	if(NOT ARKAPI_SDK_DIR)
		message(FATAL_ERROR
			"ARKAPI_SDK_DIR is not set. Point it at the public include "
			"directory of a ServerAPI checkout "
			"(https://github.com/ServersHub/ServerAPI), for example:\n"
			"  cmake -B build -DARKFIND_BUILD_PLUGIN=ON "
			"-DARKAPI_SDK_DIR=C:/ServerAPI/version/Core/Public")
	endif()

	if(NOT IS_DIRECTORY "${ARKAPI_SDK_DIR}")
		message(FATAL_ERROR
			"ARKAPI_SDK_DIR='${ARKAPI_SDK_DIR}' is not a directory.")
	endif()

	# Explicit list, matching the files that make up the plugin.
	add_library(ArkFind SHARED
		${ARKFIND_PURE_SOURCES}
		"${CMAKE_CURRENT_SOURCE_DIR}/src/Config.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/Scanner.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/Tracker.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/Commands.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/Plugin.cpp"
	)

	target_include_directories(ArkFind PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/src"
		"${ARKAPI_SDK_DIR}"
	)
	target_link_libraries(ArkFind PRIVATE arkfind_warnings)

	set_target_properties(ArkFind PROPERTIES
		OUTPUT_NAME "ArkFind"
		PREFIX ""
	)

	if(MSVC)
		# The ARK server and the SDK are built with these; a plugin has to match.
		target_compile_definitions(ArkFind PRIVATE
			WIN32_LEAN_AND_MEAN
			_CRT_SECURE_NO_WARNINGS
		)
		target_compile_options(ArkFind PRIVATE /EHsc /bigobj)
	endif()
endif()
