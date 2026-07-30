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
			"(https://github.com/Michidu/ARK-Server-API), for example:\n"
			"  cmake -B build -DARKFIND_BUILD_PLUGIN=ON "
			"-DARKAPI_SDK_DIR=C:/ARK-Server-API/version/Core/Public "
			"-DARKAPI_LIB=C:/ARK-Server-API/out_lib/ArkApi.lib")
	endif()

	if(NOT IS_DIRECTORY "${ARKAPI_SDK_DIR}")
		message(FATAL_ERROR
			"ARKAPI_SDK_DIR='${ARKAPI_SDK_DIR}' is not a directory.")
	endif()

	# ArkApi.lib exports the API entry points (GetApiUtils, GetCommands, ...).
	# It sits in out_lib/ next to the SDK's include tree, so default to that and
	# let the caller override.
	get_filename_component(ARKAPI_SDK_ROOT "${ARKAPI_SDK_DIR}/../../.." ABSOLUTE)
	set(ARKAPI_LIB "${ARKAPI_SDK_ROOT}/out_lib/ArkApi.lib" CACHE FILEPATH
		"Path to ArkApi.lib from the ARK Server API SDK")

	if(NOT EXISTS "${ARKAPI_LIB}")
		message(FATAL_ERROR
			"ARKAPI_LIB='${ARKAPI_LIB}' does not exist. Point it at out_lib/ArkApi.lib "
			"in your ARK-Server-API checkout.")
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

	# The ARK server is a release binary. A plugin built against the debug CRT
	# has a mismatched _ITERATOR_DEBUG_LEVEL and fails to load at runtime, so pin
	# the release DLL runtime regardless of the configuration being built.
	cmake_policy(SET CMP0091 NEW)
	set_property(TARGET ArkFind PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

	# The bundled Unreal headers include each other by paths relative to API/UE
	# (e.g. GenericPlatform/GenericPlatformStricmp.h does #include "CoreTypes.h"),
	# so API/UE has to be on the include path in its own right.
	target_include_directories(ArkFind PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/src"
		"${ARKAPI_SDK_DIR}"
		"${ARKAPI_SDK_DIR}/API"
		"${ARKAPI_SDK_DIR}/API/UE"
	)
	target_link_libraries(ArkFind PRIVATE arkfind_warnings "${ARKAPI_LIB}")

	set_target_properties(ArkFind PROPERTIES
		OUTPUT_NAME "ArkFind"
		PREFIX ""
	)

	if(MSVC)
		# UNICODE/_UNICODE are not optional: the SDK's Unreal headers typedef TCHAR
		# to wchar_t, and without them <windows.h> typedefs it to char, which is a
		# hard redefinition error (C2371) in BasicTypes.h. It also keeps
		# spdlog::filename_t wide, matching what Logger.h expects.
		target_compile_definitions(ArkFind PRIVATE
			UNICODE
			_UNICODE
			WIN32_LEAN_AND_MEAN
		)
		target_compile_options(ArkFind PRIVATE /EHsc /bigobj)
	endif()
endif()
