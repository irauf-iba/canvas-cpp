# draw

A small drawing, image and sound library for introductory C++ courses, inspired
by Princeton's [standard libraries](https://introcs.cs.princeton.edu/java/stdlib/).

| Module | For | Like Princeton's | Guide |
|---|---|---|---|
| `draw` | Drawing, animation, mouse and keyboard | StdDraw | [docs/draw.md](docs/draw.md) |
| `image` | Image processing, pixel by pixel | Picture | [docs/image.md](docs/image.md) |
| `audio` | Sound as samples, sound files, background sound | StdAudio | [docs/audio.md](docs/audio.md) |

```cpp
#include <draw.hpp>

int main() {
    draw::setPenColor(draw::BOOK_BLUE);
    draw::filledCircle(0.5, 0.5, 0.25);
    draw::text(0.5, 0.1, "Hello, draw!");
}
```

- **Only functions.** Everything is a free function in `namespace draw`,
  `image` or `audio`. There are no classes to construct and no objects to
  manage.
- **Nothing to set up.** The window opens on the first drawing call, and the
  sound device on the first sound.
- **Easy to install.** Students need only CMake and a C++17 compiler. SDL3 is
  downloaded and linked statically, and stays hidden from student code.
- **Clear errors.** Mistakes stop the program with a message that names the
  function, e.g. `draw: filledCircle: radius must not be negative`.
- **Ready for autograding.** Drawing is done in software, so output is the same
  everywhere. Programs can run without a window (`DRAW_HEADLESS=1`), and their
  sound can be captured to a file (`DRAW_AUDIO_CAPTURE=out.wav`).

## Using it in a project

```cmake
cmake_minimum_required(VERSION 3.24)
project(hello CXX)

include(FetchContent)
FetchContent_Declare(draw
  GIT_REPOSITORY https://github.com/<you>/stddrawlib   # TODO: real URL
  GIT_TAG        main)
FetchContent_MakeAvailable(draw)

add_executable(hello hello.cpp)
target_link_libraries(hello PRIVATE draw::draw)
```

One target, `draw::draw`, provides all three modules. A copy of this
repository next to the project works too, with `add_subdirectory(stddrawlib)`.

The first configure downloads SDL, and the first build compiles it, which takes
a minute or two. Later builds reuse it.

### Requirements

- CMake 3.24 or newer and a C++17 compiler.
- **Linux:** SDL needs development headers to build:
  - **Display:** on Debian/Ubuntu, roughly
    `libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxss-dev libxtst-dev libwayland-dev libxkbcommon-dev libdecor-0-dev libegl1-mesa-dev`.
  - **Sound:** `libasound2-dev libpulse-dev libpipewire-0.3-dev`. Without them
    SDL builds without a sound backend and programs are silent.

  SDL's `docs/README-linux.md` has the full list. The finished program loads
  X11 or Wayland, and the sound system, at run time, so it runs on any of
  them.
- **Windows and macOS:** nothing extra.

### Offline labs and system SDL

- Set `FETCHCONTENT_SOURCE_DIR_SDL3=/path/to/SDL3-3.4.16` to build from a
  local copy of the SDL source instead of downloading it.
- Configure with `-DDRAW_USE_SYSTEM_SDL=ON` to use an installed SDL 3.4 instead
  of building one.

## Building and testing this repository

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The tests need no display or speakers:

- drawing tests run headless;
- window and input tests use SDL's offscreen video driver;
- sound tests use SDL's silent `dummy` driver and its `disk` driver, which
  writes the device output to a file.

| Test | Checks |
|---|---|
| `header` | Each public header compiles on its own with strict warnings as errors. |
| `render.*` | Snapshot tests: each scene is compared with `tests/reference/<scene>.png`, allowing small differences. On failure, `<scene>-diff.png` in the build's `tests` folder marks the differing pixels in red. |
| `image` | Creating, changing, saving and loading images; `draw::canvas()` and `draw::picture()` with an image. |
| `input` | Clicks and typed keys (injected as SDL events), per-frame input rules, `pause()` timing. |
| `window.*` | The window stays open after `main()` returns, and closing it ends the program. |
| `audio` | Sound files, mixing down, sample-rate conversion, clipping and capture, without a device. |
| `audio.*` | Real-time playback, background sounds and sound at program exit, with the `dummy` driver; mixing in the device output, with the `disk` driver. |
| `error.*` | Each mistake stops the program with the expected message. |

After an intended change to rendering, regenerate the reference images, check
them by eye, and commit them:

```sh
DRAW_UPDATE_REFERENCES=1 ctest --test-dir build -R render
```

### Manual tests

Two tests need a real desktop, so `ctest` doesn't run them.

- **Real input** sends keyboard and mouse input with xdotool. It covers
  `isKeyPressed()`, typing through the OS keyboard layout, and a real window
  close request. It needs X11 or XWayland and opens a window that it types
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

For rough timings, build `--target benchmark` and run it with `DRAW_HEADLESS=1`
for drawing alone, or with a window to also measure animation frame pacing.

`DRAW_BUILD_EXAMPLES` and `DRAW_BUILD_TESTS` default to on when this is the
top-level project and off when it is used as a dependency.

## Repository layout

```
include/draw.hpp       drawing, animation and input
include/image.hpp      images as pixel grids
include/audio.hpp      sound
include/color.hpp      Color and the predefined colors (shared)
src/draw.cpp           draw implementation (SDL, rasterizer, text)
src/image.cpp          image implementation and image file I/O
src/audio.cpp          audio implementation (SDL audio, sound files)
src/common.cpp         error handling and helpers shared by the modules
src/font_data.inc      built-in font (generated by tools/make_font.sh)
docs/                  a guide to each module
examples/              shapes, bouncing_ball, sketch, image_effects, scale, piano
tests/                 tests, reference images and benchmark
tests/data/            small sound files for the audio tests
tests/manual/          real-input test (xdotool) and speaker test (parecord)
third_party/stb/       stb_truetype, stb_image, stb_image_write
third_party/dr_libs/   dr_wav 0.14.5, dr_mp3 0.7.3
third_party/font/      Noto Sans subset and its license
```

## Status

The library works and is tested on Linux (Wayland, PipeWire), both with a
window and headless. Not yet done or tested:

- Windows, macOS and high-DPI displays.
- On MSVC the library isn't yet optimized in Debug builds.

Limitations of individual modules are listed in their guides.

## Licenses

- This library: MIT ([`LICENSE`](LICENSE)).
- [SDL](https://libsdl.org): zlib license.
- [stb](https://github.com/nothings/stb): public domain or MIT.
- [dr_libs](https://github.com/mackron/dr_libs) (dr_wav, dr_mp3): public domain or MIT-0.
- Noto Sans: SIL Open Font License 1.1 (`third_party/font/OFL.txt`).
