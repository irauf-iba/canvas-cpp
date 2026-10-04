cmake_minimum_required(VERSION 3.24)
project(canvas-cpp VERSION 0.1.0 LANGUAGES C CXX)

option(CANVAS_BUILD_EXAMPLES "Build the example programs" ${PROJECT_IS_TOP_LEVEL})
option(CANVAS_BUILD_TESTS "Build the tests" ${PROJECT_IS_TOP_LEVEL})
option(CANVAS_USE_SYSTEM_SDL "Use an installed SDL3 instead of building it from source" OFF)

# ---------------------------------------------------------------------------
# SDL3 (window, input, presentation)
#
# By default SDL is downloaded and built as a static library, so students need
# only CMake and a compiler. For offline labs, set FETCHCONTENT_SOURCE_DIR_SDL3
# to an unpacked copy of the tarball below.
# ---------------------------------------------------------------------------

if(CANVAS_USE_SYSTEM_SDL)
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

  # Some compilers warn about SDL's own code (e.g. new Clang checks). Those
  # warnings aren't the student's to fix, so don't show them in their build.
  foreach(sdl_target IN ITEMS SDL3-static SDL3-shared SDL3_test SDL_uclibc)
    if(NOT TARGET ${sdl_target})
      continue()
    endif()
    if(MSVC)
      # SDL sets its own /W3; remove it, since MSVC warns (D9025) when one
      # warning level overrides another. /W0 rather than /w, because Visual
      # Studio project files only turn it into their "no warnings" setting.
      get_target_property(sdl_options ${sdl_target} COMPILE_OPTIONS)
      if(sdl_options)
        list(FILTER sdl_options EXCLUDE REGEX "/W[0-4]")
        set_property(TARGET ${sdl_target} PROPERTY COMPILE_OPTIONS "${sdl_options}")
      endif()
      target_compile_options(${sdl_target} PRIVATE /W0)
    else()
      target_compile_options(${sdl_target} PRIVATE -w)
    endif()
  endforeach()
endif()

# ---------------------------------------------------------------------------
# canvas library (the canvas, turtle, image, audio, chance, input and output modules)
# ---------------------------------------------------------------------------

add_library(canvas STATIC src/canvas.cpp src/image.cpp src/audio.cpp src/chance.cpp src/input.cpp src/turtle.cpp
                   src/common.cpp)
add_library(canvas::canvas ALIAS canvas)

target_include_directories(canvas PUBLIC include)
target_include_directories(canvas SYSTEM PRIVATE third_party/stb third_party/dr_libs)  # no warnings from these
target_compile_features(canvas PUBLIC cxx_std_17)
target_link_libraries(canvas PRIVATE SDL3::SDL3)  # students never see SDL

# Students usually build without optimization, which makes a software
# rasterizer 3-10x slower. Optimize this library regardless of their build type.
if(MSVC)
  # MSVC's Debug flags include /Od and runtime checks (/RTC1), which can't be
  # combined with /O2. Remove them for the targets in this directory only; the
  # student's own targets keep their Debug flags.
  foreach(flags CMAKE_C_FLAGS_DEBUG CMAKE_CXX_FLAGS_DEBUG)
    string(REGEX REPLACE "/RTC[1csu]+|/Od|/Ob0" "" ${flags} "${${flags}}")
  endforeach()
  target_compile_options(canvas PRIVATE $<$<CONFIG:Debug>:/O2 /Ob2>)
  # getenv and fopen are fine here; don't show "unsafe function" warnings in
  # students' build output.
  target_compile_definitions(canvas PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
  target_compile_options(canvas PRIVATE $<$<NOT:$<CONFIG:Release,RelWithDebInfo,MinSizeRel>>:-O2>)
endif()

# ---------------------------------------------------------------------------
# Examples
# ---------------------------------------------------------------------------

if(CANVAS_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

if(CANVAS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
