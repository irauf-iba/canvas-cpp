cmake_minimum_required(VERSION 3.24)
project(stddrawlib VERSION 0.1.0 LANGUAGES C CXX)

option(DRAW_BUILD_EXAMPLES "Build the example programs" ${PROJECT_IS_TOP_LEVEL})
option(DRAW_BUILD_TESTS "Build the tests" ${PROJECT_IS_TOP_LEVEL})
option(DRAW_USE_SYSTEM_SDL "Use an installed SDL3 instead of building it from source" OFF)

# ---------------------------------------------------------------------------
# SDL3 (window, input, presentation)
#
# By default SDL is downloaded and built as a static library, so students need
# only CMake and a compiler. For offline labs, set FETCHCONTENT_SOURCE_DIR_SDL3
# to an unpacked copy of the tarball below.
# ---------------------------------------------------------------------------

if(DRAW_USE_SYSTEM_SDL)
  find_package(SDL3 3.4 REQUIRED CONFIG)
else()
  # Static only, and skip the subsystems we never use. These are cache
  # defaults, so -D on the command line still overrides them.
  set(SDL_STATIC       ON  CACHE BOOL "")
  set(SDL_SHARED       OFF CACHE BOOL "")
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "")
  set(SDL_INSTALL      OFF CACHE BOOL "")
  set(SDL_GPU          OFF CACHE BOOL "")
  set(SDL_CAMERA       OFF CACHE BOOL "")
  set(SDL_JOYSTICK     OFF CACHE BOOL "")
  set(SDL_HIDAPI       OFF CACHE BOOL "")
  set(SDL_HAPTIC       OFF CACHE BOOL "")
  set(SDL_SENSOR       OFF CACHE BOOL "")
  set(SDL_POWER        OFF CACHE BOOL "")
  set(SDL_DIALOG       OFF CACHE BOOL "")
  set(SDL_TRAY         OFF CACHE BOOL "")
  set(SDL_VULKAN       OFF CACHE BOOL "")
  set(SDL_RENDER_D3D12 OFF CACHE BOOL "")
  # SDL_AUDIO stays on for the planned audio module.

  # CMake 4.4 warns about backslashes in one of SDL's own configure checks.
  # Keep the behavior SDL was written for, and keep the output quiet.
  set(CMAKE_POLICY_DEFAULT_CMP0219 OLD)

  include(FetchContent)
  FetchContent_Declare(
    SDL3
    URL      https://github.com/libsdl-org/SDL/releases/download/release-3.4.16/SDL3-3.4.16.tar.gz
    URL_HASH SHA256=7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(SDL3)
endif()

# ---------------------------------------------------------------------------
# draw library (the draw and image modules)
# ---------------------------------------------------------------------------

add_library(draw STATIC src/draw.cpp src/image.cpp)
add_library(draw::draw ALIAS draw)

target_include_directories(draw PUBLIC include)
target_include_directories(draw SYSTEM PRIVATE third_party/stb)  # no warnings from stb
target_compile_features(draw PUBLIC cxx_std_17)
target_link_libraries(draw PRIVATE SDL3::SDL3)  # students never see SDL

# Students usually build without optimization, which makes a software
# rasterizer 5-10x slower. Optimize this library regardless of their build type.
# (MSVC needs /RTC1 removed before /O2 can be added; not handled yet.)
if(NOT MSVC)
  target_compile_options(draw PRIVATE $<$<NOT:$<CONFIG:Release,RelWithDebInfo,MinSizeRel>>:-O2>)
endif()

# ---------------------------------------------------------------------------
# Examples
# ---------------------------------------------------------------------------

if(DRAW_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

if(DRAW_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
