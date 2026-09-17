cmake_minimum_required(VERSION 3.16)
project(lendiza VERSION 0.1.0 LANGUAGES C CXX)

# Require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compile sources into object libraries for explicit release/debug variants.
# This lets us build two static libraries with different compile flags in the same build.
add_library(lendiza_objs_release OBJECT
    src/lendiza.cpp
)
add_library(lendiza_objs_debug OBJECT
    src/lendiza.cpp
)

# Common include directories for the object libraries
target_include_directories(lendiza_objs_release
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_include_directories(lendiza_objs_debug
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Also require C++20 on object targets (defensive)
target_compile_features(lendiza_objs_release PUBLIC cxx_std_20)
target_compile_features(lendiza_objs_debug PUBLIC cxx_std_20)

# Set compile options to approximate Release and Debug builds for common compilers
if(MSVC)
    # Apply flags per-configuration to avoid mixing incompatible MSVC options
    target_compile_options(lendiza_objs_release PRIVATE
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:RelWithDebInfo>:/O2>
        $<$<CONFIG:MinSizeRel>:/O2>
        $<$<CONFIG:Release>:/DNDEBUG>
        $<$<CONFIG:RelWithDebInfo>:/DNDEBUG>
        $<$<CONFIG:MinSizeRel>:/DNDEBUG>
    )
    target_compile_options(lendiza_objs_debug PRIVATE
        $<$<CONFIG:Debug>:/Zi>
        $<$<CONFIG:Debug>:/Od>
        $<$<CONFIG:Debug>:/DDEBUG>
    )
else()
    # GCC/Clang flags
    target_compile_options(lendiza_objs_release PRIVATE -O3 -DNDEBUG)
    target_compile_options(lendiza_objs_debug PRIVATE -g -O0 -DDEBUG)
endif()

# Now create two static libraries from those object libraries
add_library(lendiza STATIC $<TARGET_OBJECTS:lendiza_objs_release>)
add_library(lendiza_d STATIC $<TARGET_OBJECTS:lendiza_objs_debug>)

# Install/include configuration for both libraries
target_include_directories(lendiza
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/lendiza>
)

target_include_directories(lendiza_d
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/lendiza>
)

# Prefer consistent library filenames: lendiza(.lib) and lendiza_d(.lib) on Windows
set_target_properties(lendiza PROPERTIES OUTPUT_NAME "lendiza" ARCHIVE_OUTPUT_NAME "lendiza")
set_target_properties(lendiza_d PROPERTIES OUTPUT_NAME "lendiza_d" ARCHIVE_OUTPUT_NAME "lendiza_d")

# Export both targets and install both static libraries
install(TARGETS lendiza lendiza_d
    EXPORT lendizaTargets
    ARCHIVE DESTINATION lib/lendiza
)

install(DIRECTORY include/
    DESTINATION include/lendiza
)

install(EXPORT lendizaTargets
    FILE lendizaTargets.cmake
    NAMESPACE lendiza::
    DESTINATION lib/cmake/lendiza
)

# Generate and install CMake package config files so find_package(lendiza CONFIG REQUIRED)
include(CMakePackageConfigHelpers)

# Configure a minimal Config file that includes the installed targets file
# The input template (cmake/lendizaConfig.cmake.in) will be configured into the build dir
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/lendizaConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/lendizaConfig.cmake"
    INSTALL_DESTINATION "lib/cmake/lendiza"
)

# Write a simple version file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/lendizaConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# Install the generated config files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/lendizaConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/lendizaConfigVersion.cmake"
    DESTINATION lib/cmake/lendiza
)

# Build example executables for local testing but do NOT install them.
# Examples are always built so you can run them locally, but they are not part of the install.
add_executable(lendiza_example_c examples/example.c)
set_target_properties(lendiza_example_c PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(lendiza_example_c PRIVATE lendiza)
target_include_directories(lendiza_example_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(lendiza_example_cpp examples/example.cpp)
target_link_libraries(lendiza_example_cpp PRIVATE lendiza)
target_include_directories(lendiza_example_cpp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ---------------------------------------------------------------------------
# Tests and kernel-safety probes.
#
# Only built when lendiza is the top-level project, so downstream submodule
# users do not inherit a test dependency.  The suites use the dependency-free
# harness in tests/lendiza_test.h (GoogleTest-shaped API).
# ---------------------------------------------------------------------------
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    option(LENDIZA_BUILD_TESTS "Build the lendiza test suites and probes" ON)
else()
    option(LENDIZA_BUILD_TESTS "Build the lendiza test suites and probes" OFF)
endif()

if(LENDIZA_BUILD_TESTS)
    enable_testing()
    file(GLOB LENDIZA_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_*.cpp")

    add_executable(lendiza_tests ${LENDIZA_TEST_SOURCES} src/lendiza.cpp)
    target_include_directories(lendiza_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    # The suites include lendiza.hpp (std::span), so they need C++20 even though
    # the kernel-safe core only needs C++17.
    target_compile_features(lendiza_tests PRIVATE cxx_std_20)
    target_compile_definitions(lendiza_tests PRIVATE
        LENDIZA_ORACLE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/tests/data/oracle_before.bin"
        LENDIZA_WALK_PATH="${CMAKE_CURRENT_SOURCE_DIR}/tests/data/walk_before.txt"
        LENDIZA_GROUP_BASELINE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/tests/data/group_before.bin")
    if(MSVC)
        target_compile_options(lendiza_tests PRIVATE /W4)
    else()
        target_compile_options(lendiza_tests PRIVATE -Wall -Wextra)
    endif()
    add_test(NAME lendiza_tests COMMAND lendiza_tests)

    # Proves no read happens past the bytes handed to the decoder, by placing
    # every input against an inaccessible page.
    add_executable(lendiza_guard_page tests/guard_page.cpp)
    target_include_directories(lendiza_guard_page PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    target_compile_features(lendiza_guard_page PRIVATE cxx_std_17)
    add_test(NAME lendiza_guard_page COMMAND lendiza_guard_page)

    # The same property checked by AddressSanitizer through the extern "C" ABI.
    add_executable(lendiza_asan_truncation tests/asan_truncation.c src/lendiza.cpp)
    target_include_directories(lendiza_asan_truncation PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    add_test(NAME lendiza_asan_truncation COMMAND lendiza_asan_truncation)

    # Records the group/x87 sweep baseline (see tests/tools/dump_group_baseline.cpp)
    add_executable(lendiza_dump_group_baseline tests/tools/dump_group_baseline.cpp)
    target_include_directories(lendiza_dump_group_baseline PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    target_compile_features(lendiza_dump_group_baseline PRIVATE cxx_std_20)

    # The two safety probes run millions of decodes; give them room and let
    # `ctest -L quick` stay fast.
    set_tests_properties(lendiza_tests PROPERTIES LABELS quick TIMEOUT 600)
    set_tests_properties(lendiza_guard_page PROPERTIES LABELS "kernel-safety" TIMEOUT 3600)
    set_tests_properties(lendiza_asan_truncation PROPERTIES LABELS "kernel-safety" TIMEOUT 3600)

    if(LENDIZA_SANITIZE)
        foreach(t lendiza_asan_truncation)
            target_compile_options(${t} PRIVATE
                $<$<CXX_COMPILER_ID:GNU,Clang>:-fsanitize=address>
                $<$<C_COMPILER_ID:GNU,Clang>:-fsanitize=address>
                $<$<CXX_COMPILER_ID:MSVC>:/fsanitize=address>
                $<$<C_COMPILER_ID:MSVC>:/fsanitize=address>)
            target_link_options(${t} PRIVATE
                $<$<CXX_COMPILER_ID:GNU,Clang>:-fsanitize=address>
                $<$<C_COMPILER_ID:GNU,Clang>:-fsanitize=address>)
        endforeach()
    endif()
endif()

# ---------------------------------------------------------------------------
# lde64 comparison benchmark.
#
# Off by default: it links a prebuilt third-party archive and reads .text
# corpora that both live outside this repository, so the two paths below are
# the supported way to point at them.
#
# GNU toolchain only - bench/lde64_stub.s is GAS syntax. MSVC would need an
# MASM equivalent of the same eight instructions.
# ---------------------------------------------------------------------------
option(LENDIZA_BUILD_BENCH "Build the lendiza vs lde64 length-decode benchmark" OFF)

if(LENDIZA_BUILD_BENCH)
    if(MSVC)
        message(FATAL_ERROR "LENDIZA_BUILD_BENCH needs the GNU toolchain (bench/lde64_stub.s is GAS syntax)")
    endif()
    enable_language(ASM)

    set(LENDIZA_LDE64_LIB "C:/devdrv/src/hypermon/third_party/lde64/LDE64x64.lib"
        CACHE FILEPATH "Prebuilt lde64 static library")
    set(LENDIZA_BENCH_FIXTURE_DIR "C:/devdrv/src/xendiza/bench/fixtures"
        CACHE PATH "Directory containing the *.text.bin length-decode corpora")

    if(NOT EXISTS "${LENDIZA_LDE64_LIB}")
        message(FATAL_ERROR "LENDIZA_LDE64_LIB not found: ${LENDIZA_LDE64_LIB}")
    endif()

    add_executable(lendiza_bench bench/bench_lde.cpp bench/lde64_stub.s src/lendiza.cpp)
    target_include_directories(lendiza_bench PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_compile_features(lendiza_bench PRIVATE cxx_std_20)
    target_compile_definitions(lendiza_bench PRIVATE
        LENDIZA_BENCH_FIXTURE_DIR="${LENDIZA_BENCH_FIXTURE_DIR}")
    # The archive exports the C symbol LDE; no decoration on either toolchain's
    # x64 ABI, so it links as-is under MinGW and MSVC alike.
    target_link_libraries(lendiza_bench PRIVATE "${LENDIZA_LDE64_LIB}")
endif()
