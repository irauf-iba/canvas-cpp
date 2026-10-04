# canvas

A small library for introductory C++ courses, for drawing, turtle graphics, images, sound, random numbers and reading input, inspired
by Princeton's [standard libraries](https://introcs.cs.princeton.edu/java/stdlib/).

| Module | For | Like Princeton's | Guide |
|---|---|---|---|
| `canvas` | Drawing, animation, mouse and keyboard | StdDraw | [docs/canvas.md](docs/canvas.md) |
| `turtle` | Drawing by moving and turning, for loops and recursion | Turtle | [docs/turtle.md](docs/turtle.md) |
| `image` | Image processing, pixel by pixel | Picture | [docs/image.md](docs/image.md) |
| `audio` | Sound as samples, sound files, background sound | StdAudio | [docs/audio.md](docs/audio.md) |
| `chance` | Random numbers, the same on every computer for a given seed | StdRandom | [docs/chance.md](docs/chance.md) |
| `input`, `output` | Help with `std::cin`: checked questions, reading all values, Windows line endings; files in an IDE | StdIn, StdOut | [docs/input.md](docs/input.md) |

**Students:** copy the starter project in [`template`](template) and follow
its one-page [README](template/README.md). The full
[getting-started guide](docs/getting-started.md) covers Visual Studio,
VS Code, CLion and the command line in more detail.

```cpp
#include <canvas.hpp>

int main() {
    canvas::setPenColor(canvas::BOOK_BLUE);
    canvas::filledCircle(0.5, 0.5, 0.25);
    canvas::text(0.5, 0.1, "Hello, canvas!");
}
```

- **Only functions.** Everything is a free function in `namespace canvas`,
  `turtle`, `image`, `audio`, `chance`, `input` or `output`. There are no classes to construct and no objects to
  manage.
- **Nothing to set up.** The window opens on the first drawing call, and the
  sound device on the first sound.
- **Easy to install.** Students need only CMake and a C++17 compiler. SDL3 is
  downloaded and linked statically, and stays hidden from student code.
- **Clear errors.** Mistakes stop the program with a message that names the
  function, e.g. `canvas: filledCircle: radius must not be negative`.
- **Ready for autograding.** Drawing is done in software, so output is the same
  everywhere. Programs can run without a window (`CANVAS_HEADLESS=1`), their
  sound can be captured to a file (`CANVAS_AUDIO_CAPTURE=out.wav`), and their
  random numbers repeated (`CANVAS_SEED=42`).

## Using it in a project

The [`template`](template) folder is a ready-made project. To add the library
to your own:

```cmake
cmake_minimum_required(VERSION 3.24)
project(hello CXX)

include(FetchContent)
FetchContent_Declare(canvas
  GIT_REPOSITORY https://github.com/irauf-iba/canvas-cpp
  GIT_TAG        main
  GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(canvas)

add_executable(hello hello.cpp)
target_link_libraries(hello PRIVATE canvas::canvas)
```

One target, `canvas::canvas`, provides all the modules. A copy of this
repository next to the project works too, with `add_subdirectory(canvas-cpp)`.

The first configure downloads SDL, and the first build compiles it, which takes
a minute or two. Later builds reuse it.

### Requirements

- CMake 3.24 or newer and a C++17 compiler.
- **Linux:** SDL needs `pkg-config` and development headers to build. The
  Debian/Ubuntu package list is in
  [docs/getting-started.md](docs/getting-started.md#1-install-the-tools)
  (the same list the CI uses). Without the sound packages SDL builds without a
  sound backend and programs are silent. SDL's `docs/README-linux.md` has
  details. The finished program loads
  X11 or Wayland, and the sound system, at run time, so it runs on any of
  them.
- **Windows and macOS:** nothing extra.

### Offline labs and system SDL

- Set `FETCHCONTENT_SOURCE_DIR_SDL3=/path/to/SDL3-3.4.16` to build from a
  local copy of the SDL source instead of downloading it.
- Configure with `-DCANVAS_USE_SYSTEM_SDL=ON` to use an installed SDL 3.4 instead
  of building one.

## Building and testing this repository

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The tests need no display or speakers:

- drawing tests run headless;
- window and input tests use SDL's `dummy` video driver;
- sound tests use SDL's silent `dummy` driver and its `disk` driver, which
  writes the device output to a file.

| Test | Checks |
|---|---|
| `header` | Each public header compiles on its own with strict warnings as errors. |
| `render.*` | Snapshot tests: each scene is compared with `tests/reference/<scene>.png`, allowing small differences. On failure, `<scene>-diff.png` in the build's `tests` folder marks the differing pixels in red. |
| `image` | Creating, changing, saving and loading images; `canvas::snapshot()` and `canvas::picture()` with an image. |
| `text` | `text()` with numbers and characters: formatting and choice of overload. |
| `turtle` | Turtle positions and headings (including wrapping past 360°), the pen, and a square closing exactly; plus the `render.turtle` snapshot. |
| `reading`, `reading.stdin`, `ask*`, `output` | `fromFile` with plain `std::cin` (Windows line endings, byte-order mark), `skipRestOfLine`, `skipEmptyLines`, `getLine` and `readAll…` on files and real standard input, the exact prompts and messages of the `ask…` functions, the string helpers, and `output::toFile`. |
| `chance`, `chance.*` | The same numbers for a seed (pinned values, checked on every platform), ranges, distributions, shuffling, and `CANVAS_SEED`. |
| `lookup` | Files are found next to the program when the current folder is elsewhere. |
| `input` | Clicks, typed keys and `wasKeyPressed` (injected as SDL events), per-frame input rules, `pause()` timing. |
| `window.*` | The window stays open after `main()` returns, and closing it ends the program. |
| `audio` | Sound files, mixing down, sample-rate conversion, clipping and capture, without a device. |
| `audio.*` | Real-time playback, background sounds, sound at program exit, and closing the window while that sound finishes, with the `dummy` driver; mixing in the device output, with the `disk` driver. |
| `error.*` | Each mistake stops the program with the expected message. |

After an intended change to rendering, regenerate the reference images, check
them by eye, and commit them:

```sh
CANVAS_UPDATE_REFERENCES=1 ctest --test-dir build -R render
```

On GitHub, the workflow in `.github/workflows/ci.yml` runs this build and all
tests on Linux, Windows (MSVC) and macOS, in Debug and Release. If a snapshot
test fails there, its `-diff.png` images are attached to the run.

### Manual tests

Two tests need a real desktop, so `ctest` doesn't run them.

- **Real input** sends keyboard and mouse input with xdotool. It covers
  `isKeyPressed()`, `wasKeyPressed()` with a quick tap, typing through the OS
  keyboard layout, and a real window close request. It needs X11 or XWayland and opens a window that it types
  into, so don't type while it runs. It checks before every keystroke that the
  test window has focus, and stops otherwise.

  ```sh
  cmake --build build --target real_input
  tests/manual/real_input.sh build/tests/real_input
  ```

- **Speakers** plays about 3.5 seconds of tones through the output device,
  records what reaches it, and checks the length, pitch and mixing. It needs
  PipeWire or PulseAudio, with `pactl` and `parecord`.

  ```sh
  cmake --build build --target speaker_test
  tests/manual/speaker_test.sh build/tests/speaker_test
  ```

For rough timings, build `--target benchmark` and run it with `CANVAS_HEADLESS=1`
for drawing alone, or with a window to also measure animation frame pacing.

`CANVAS_BUILD_EXAMPLES` and `CANVAS_BUILD_TESTS` default to on when this is the
top-level project and off when it is used as a dependency.

## Repository layout

```
include/canvas.hpp     drawing, animation and input
include/turtle.hpp     turtle graphics
include/image.hpp      images as pixel grids
include/audio.hpp      sound
include/chance.hpp     random numbers
include/input.hpp      helpers for std::cin (and output.hpp: std::cout to a file)
include/color.hpp      Color and the predefined colors (shared)
src/canvas.cpp         canvas implementation (SDL, rasterizer, text)
src/turtle.cpp         turtle implementation (draws with canvas)
src/image.cpp          image implementation and image file I/O
src/audio.cpp          audio implementation (SDL audio, sound files)
src/chance.cpp         chance implementation
src/input.cpp          input and output implementation
src/common.cpp         error handling, file lookup and other shared helpers
src/font_data.inc      built-in font (generated by tools/make_font.sh)
docs/                  getting started, and a guide to each module
template/              starter project for students
examples/              shapes, bouncing_ball, sketch, image_effects, scale, piano, random_walk,
                       average, grades, guess, koch, tree (data files in examples/data/)
tests/                 tests, reference images and benchmark
tests/data/            small sound files for the audio tests
tests/manual/          real-input test (xdotool) and speaker test (parecord)
third_party/stb/       stb_truetype, stb_image, stb_image_write
third_party/dr_libs/   dr_wav 0.14.5, dr_mp3 0.7.3
third_party/font/      Noto Sans subset and its license
.github/workflows/     CI: build and test on Linux, Windows and macOS
```

## Status

The library works and is tested on Linux (Wayland, PipeWire), both with a
window and headless. The CI workflow also builds and tests on Windows (MSVC)
and macOS; see the repository's Actions tab for the latest results. Not yet
tested:

- Windows and macOS beyond what the CI covers: real windows, high-DPI
  displays and sound devices.
- The MSVC settings that optimize the library in Debug builds.
- The IDE steps in the getting-started guide, except on Linux.

Limitations of individual modules are listed in their guides.

## Licenses

- This library: MIT ([`LICENSE`](LICENSE)).
- [SDL](https://libsdl.org): zlib license.
- [stb](https://github.com/nothings/stb): public domain or MIT.
- [dr_libs](https://github.com/mackron/dr_libs) (dr_wav, dr_mp3): public domain or MIT-0.
- Noto Sans: SIL Open Font License 1.1 (`third_party/font/OFL.txt`).
