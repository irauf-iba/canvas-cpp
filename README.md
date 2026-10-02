# draw

A small drawing, image and sound library for introductory C++ courses, inspired
by Princeton's [standard libraries](https://introcs.cs.princeton.edu/java/stdlib/)
(StdDraw, Picture and StdAudio). It has three modules: `draw` for drawing and
animation, `image` for pixel-level image processing, and `audio` for sound.

```cpp
#include <draw.hpp>

int main() {
    draw::setPenColor(draw::BOOK_BLUE);
    draw::filledCircle(0.5, 0.5, 0.25);
    draw::text(0.5, 0.1, "Hello, draw!");
}
```

- **Only functions.** Everything is a free function in `namespace draw`. There
  are no classes to construct and no objects to manage.
- **Nothing to set up.** The window opens on the first drawing call and stays
  open after `main()` returns, until the user closes it.
- **Easy to install.** Students need only CMake and a C++17 compiler. SDL3 is
  downloaded and linked statically, and stays hidden from student code.
- **Same output everywhere.** Drawing is done in software into an in-memory
  canvas. A headless mode writes PNGs without a window, and sound can be
  captured to a WAV file, for autograding.

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

A copy of this repository next to the project works too, with
`add_subdirectory(stddrawlib)`.

The first configure downloads SDL, and the first build compiles it, which takes
a minute or two. Later builds reuse it.

### Requirements

- CMake 3.24 or newer and a C++17 compiler.
- **Linux:** SDL needs the X11 and Wayland development headers to build. On
  Debian/Ubuntu that is roughly
  `libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxss-dev libxtst-dev libwayland-dev libxkbcommon-dev libdecor-0-dev libegl1-mesa-dev`.
  For sound it also needs `libasound2-dev libpulse-dev libpipewire-0.3-dev`.
  Without them SDL builds without a sound backend and programs are silent.
  SDL's `docs/README-linux.md` has the full list. The finished program loads
  X11 or Wayland, and the sound system, at run time, so it runs on any of them.
- **Windows and macOS:** nothing extra.

### Offline labs and system SDL

- Set `FETCHCONTENT_SOURCE_DIR_SDL3=/path/to/SDL3-3.4.16` to build from a
  local copy of the SDL source instead of downloading it.
- Configure with `-DDRAW_USE_SYSTEM_SDL=ON` to use an installed SDL 3.4 instead
  of building one.

## The API at a glance

The full documentation is in [`include/draw.hpp`](include/draw.hpp).

| Area | Functions |
|---|---|
| Window | `setCanvasSize(w, h)`, `setTitle(s)` |
| Coordinates | `setXscale(min, max)`, `setYscale(min, max)`, `setScale(min, max)` |
| Pen | `setPenColor(color)`, `setPenColor(r, g, b)`, `penColor()`, `setPenWidth(px)`, `penWidth()` |
| Colors | `draw::Color{r, g, b, a}`, `draw::rgb(r, g, b)`, `BLACK`, `BOOK_BLUE`, … |
| Shapes | `point`, `line`, `circle`, `ellipse`, `arc`, `square`, `rectangle`, `polygon`, `polyline` |
| Filled shapes | `filledCircle`, `filledEllipse`, `filledSquare`, `filledRectangle`, `filledPolygon` |
| Text | `text(x, y, s)`, `textLeft`, `textRight`, `text(x, y, s, degrees)`, `setFont(ttf)`, `setFontSize(px)` |
| Images | `picture(x, y, file)`, `picture(x, y, file, w, h)`, `picture(x, y, img)`, `save(file)`, `canvas()` |
| Animation | `clear()`, `enableDoubleBuffering()`, `show()`, `pause(ms)` |
| Mouse | `mouseX()`, `mouseY()`, `isMousePressed()`, `mouseClicked()` |
| Keyboard | `hasNextKeyTyped()`, `nextKeyTyped()`, `isKeyPressed(draw::Key::Left)` |

By default the canvas is 512 × 512 pixels, with coordinates from (0, 0) at the
lower left to (1, 1) at the upper right. Pen widths and font sizes are in
pixels, so they don't change when the scale does.

Circles, squares and arcs are always round and square, even on a rectangular
canvas or when the x and y scales differ, as in a plot. Their radius or
half-length is measured in x units and used in both directions. `ellipse()` and
`rectangle()` take a separate width and height, and follow each axis's scale.

### Animation

```cpp
draw::enableDoubleBuffering();
while (true) {
    draw::clear();
    // ...draw the next frame...
    draw::show();
    draw::pause(16);   // about 60 frames per second
}
```

The time spent drawing counts toward `pause()`, so frames stay evenly spaced.
A one-off `pause(2000)` still waits two full seconds.

### Input happens per frame

`show()` and `pause()` mark the end of a frame. Input that the program didn't
read during a frame is dropped when the next frame starts, so a program that
falls behind never reacts to old clicks or keys.

- `mouseClicked()` is true once for each click in the current frame.
- Typed characters wait in a buffer of up to 16 unread characters. Use `if`
  to handle one key per frame, or `while` to handle all of them:

  ```cpp
  while (draw::hasNextKeyTyped()) {
      char c = draw::nextKeyTyped();   // ASCII; Enter is '\n', Backspace '\b'
  }
  ```

- `isKeyPressed()` and `isMousePressed()` report what is held down right now.
  Use them for game controls.

### Images: `image.hpp`

The `image` module covers image-processing exercises, like Princeton's
`Picture`. An `image::Image` is a plain struct with `width`, `height` and
`pixels`. You can copy it, pass it to functions and return it. It needs no
window.

```cpp
#include <draw.hpp>
#include <image.hpp>

image::Image src = image::load("photo.png");
image::Image out = image::create(src.width, src.height);
for (int row = 0; row < src.height; ++row)
    for (int col = 0; col < src.width; ++col) {
        image::Color c = image::get(src, col, row);
        int gray = (299 * c.r + 587 * c.g + 114 * c.b) / 1000;
        image::set(out, col, row, image::rgb(gray, gray, gray));
    }
image::save(out, "gray.png");

draw::setCanvasSize(out.width, out.height);   // or show it
draw::picture(0.5, 0.5, out);
```

| Function | Does |
|---|---|
| `image::create(w, h)`, `image::create(w, h, color)` | A new image, white or filled with a color. |
| `image::load(file)`, `image::save(img, file)` | Read or write .png, .jpg or .bmp (.gif can be read). |
| `image::get(img, col, row)`, `image::set(img, col, row, color)` | Read or change one pixel, with a clear error if it is outside the image. |
| `draw::picture(x, y, img)`, `draw::picture(x, y, img, w, h)` | Draw an image on the canvas. At natural size each image pixel covers exactly one canvas pixel. |
| `draw::canvas()` | A copy of the canvas as an image, e.g. to process a drawing. |

- **Rows and columns.** Column 0 is at the left and row 0 at the top, as in
  image files and image editors. This is the opposite of `draw`'s y axis,
  which points up; rows are positions in a grid, not coordinates.
- **Colors.** `draw::Color` and `image::Color` are the same type, from
  `color.hpp`, as are the color constants (`draw::RED` is `image::RED`).
  Colors can be compared with `==`.

`examples/image_effects.cpp` shows a grayscale and a mirrored copy side by
side, of a drawing it makes itself or of an image file you pass to it.

### Sound: `audio.hpp`

The `audio` module works like Princeton's `StdAudio`. Sound is a sequence of
samples, numbers from −1 to +1, at `audio::SAMPLE_RATE` (44,100) samples per
second.

```cpp
#include <audio.hpp>

const double PI = 3.14159265358979323846;
for (int i = 0; i < audio::SAMPLE_RATE; ++i)          // one second of A (440 Hz)
    audio::play(0.5 * std::sin(2 * PI * 440 * i / audio::SAMPLE_RATE));
```

| Function | Does |
|---|---|
| `audio::play(sample)`, `audio::play(samples)` | Play samples. Waits when about 0.1 s is queued, so a loop playing one sample at a time runs in real time. |
| `audio::drain()` | Wait until everything queued has played. |
| `audio::read(file)`, `audio::save(file, samples)` | Read a .wav or .mp3 file (mixed to mono, converted to 44,100 Hz); write a 16-bit mono .wav file. |
| `audio::play(file)` | Play a .wav or .mp3 file and wait until it has finished. |
| `audio::playInBackground(file or samples)`, `audio::loopInBackground(file)`, `audio::stopInBackground()` | Sound effects and music for games. These return immediately, and sounds are mixed together. |

- **Program end.** Sound queued with `play()` finishes playing before the
  program ends, so the last note isn't cut off. Background sounds stop.
- **With `draw`.** While `play()` waits, the window stays responsive. Input
  queries such as `draw::hasNextKeyTyped()` cost about 50 ns, so a Guitar
  Hero-style program can check the keyboard once per sample.
- **No sound device.** If there is none, for example over SSH, a warning is
  printed once and the program runs without sound.
- **Examples.** `examples/scale.cpp` plays a scale one sample at a time and
  saves it. `examples/piano.cpp` is a keyboard piano that combines `draw`
  input with background sound.

### Mistakes stop the program with a message

A negative radius, a NaN coordinate, a missing image file, a pixel outside an
image and similar mistakes print a message and exit with status 1, for example
`draw: filledCircle: radius must not be negative` or
`image: get: col 640 is outside the image (0 to 639)`.
Sound file errors read like `audio: read: cannot open 'tune.wav'`.

## Differences from Java's StdDraw

| StdDraw | draw | Why |
|---|---|---|
| `setPenRadius(0.002)` | `setPenWidth(2)` | Pixels are easier to reason about, and widths don't change with the scale. |
| `getPenColor()` | `penColor()` | Shorter and C++-style. |
| `new Color(r, g, b)` | `draw::rgb(r, g, b)` | Accepts `int` variables (`Color{r, g, b}` doesn't) and clamps to 0–255. |
| `double[] x, double[] y` | `{{x, y}, ...}` or two vectors | `draw::Point` lists read naturally, e.g. `polygon({{0, 0}, {1, 0}, {0.5, 1}})`. |
| — | `polyline(points)` | For plotting functions. |
| `isMousePressed()` polling for clicks | `mouseClicked()` | Short clicks between two polls aren't missed. |
| Typed keys queue forever | Dropped at the end of each frame | A lagging program doesn't replay old keys. |
| Circles and squares stretch when the x and y scales differ | Always round and square; size in x units | A circle should look like a circle, and plot markers stay dots. |
| `pause(ms)` sleeps | `pause(ms)` keeps a steady frame rate | Smooth animation even when drawing takes time. |
| `Picture` class with `get`/`set` methods | `image::Image` struct with `image::get`/`image::set` | Same idea without classes; images are values you can copy and return. |
| StdAudio reads WAV, AU, AIFF and MIDI | `audio` reads WAV and MP3 | MP3 is what students have; MIDI and recording are not supported. |
| Exceptions | Message and exit | Clearer for beginners than an uncaught exception. |

## Autograding

Set `DRAW_HEADLESS=1` to draw without a window. Drawing works as usual,
`pause()` returns immediately, no input is ever reported, and `save("out.png")`
writes the canvas. Student code needs no changes:

```sh
DRAW_HEADLESS=1 ./student_program && compare out.png expected.png
```

`save()` always writes the logical canvas size, even on high-DPI screens, so
output is the same on every machine.

For sound, set `DRAW_AUDIO_CAPTURE=out.wav`. Everything passed to
`audio::play()` is then also written to `out.wav` when the program ends.
Background sounds are not included, since their timing depends on the
machine. In headless mode no audio device is opened and `play()` returns
immediately, so a program that plays a minute of music finishes in moments:

```sh
DRAW_HEADLESS=1 DRAW_AUDIO_CAPTURE=out.wav ./student_program
```

## Building and testing this repository

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The tests need no display: drawing tests run headless, and window and input
tests use SDL's offscreen video driver.

| Test | Checks |
|---|---|
| `header` | Each public header compiles on its own with strict warnings as errors; `rgb()` clamps. |
| `render.*` | Snapshot tests: each scene is compared with `tests/reference/<scene>.png`, allowing small differences. On failure, `<scene>-diff.png` in the build's `tests` folder marks the differing pixels in red. |
| `image` | Creating, reading, changing, saving and loading images; copies are independent; `draw::canvas()` and `draw::picture()` with an image (pixel-exact at natural size). |
| `input` | Clicks and typed keys (injected as SDL events), per-frame input rules, `pause()` timing. |
| `window.*` | The window stays open after `main()` returns, and closing it ends the program, including in the middle of drawing. An error in the image module closes the window. |
| `audio` | Without a device: WAV round trip, clipping, stereo-to-mono and 22,050 → 44,100 Hz conversion, MP3 decoding, headless `play()` not waiting, and the capture file matching what was played. |
| `audio.*` | With SDL's silent `dummy` driver: real-time pacing of one-sample-at-a-time `play()` with a `draw` input query per sample, `play(file)` and `drain()` timing, background sounds, and queued sound finishing at exit. With the `disk` driver: `play()` and a background sound mixed in the device output. |
| `error.*` | Each mistake stops the program with the expected message. |

After an intended change to rendering, regenerate the reference images, check
them by eye, and commit them:

```sh
DRAW_UPDATE_REFERENCES=1 ctest --test-dir build -R render
```

One test needs a real desktop, so `ctest` doesn't run it. It sends real
keyboard and mouse input with xdotool, covering `isKeyPressed()`, typing
through the OS keyboard layout, and a real window close request:

```sh
cmake --build build --target real_input
tests/manual/real_input.sh build/tests/real_input
```

It needs xdotool and X11 or XWayland; on Wayland it runs the program with
`SDL_VIDEO_DRIVER=x11`. It opens a window and types into it, so don't type
while it runs. It checks before every keystroke that the test window has
focus, and stops otherwise.

For rough timings, build the benchmark with
`cmake --build build --target benchmark`. Run it with `DRAW_HEADLESS=1` to time
drawing alone, or with a window to also measure animation frame pacing.

Options: `DRAW_BUILD_EXAMPLES` and `DRAW_BUILD_TESTS` default to on when this
is the top-level project and off when it is used as a dependency.

## How it works

- **Drawing.** Each call rasterizes straight into an RGBA canvas, on the
  caller's thread. There is no render thread and no list of shapes.
- **Filled shapes** use exact-area coverage, so edges are antialiased and
  self-crossing polygons fill by the nonzero rule.
- **Lines and outlines** take their coverage from the distance to the line
  segments. That gives round joins and caps, and overlapping pieces join
  without visible seams.
- **Displaying.** SDL copies the canvas to a window texture:
  - in double-buffered mode, on `show()`, with vsync;
  - otherwise automatically, at most about 60 times a second.
- **Fonts and images.** Text uses stb_truetype with a built-in subset of Noto
  Sans. The image module reads and writes files with stb_image and
  stb_image_write; it doesn't use SDL.
- **Sound.** `play()` feeds an SDL audio stream from the caller's thread, and
  each background sound has its own stream. SDL mixes them and converts to the
  device's format. Looping sounds are refilled from SDL's audio thread. Sound
  files are read with dr_wav and dr_mp3.
- **Speed.** The library is compiled with optimization even when the student's
  project builds in Debug.

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
examples/              shapes, bouncing_ball, sketch, image_effects, scale, piano
tests/                 tests, reference images and benchmark
tests/data/            small sound files for the audio tests
tests/manual/          real-input test driven by xdotool
third_party/stb/       stb_truetype, stb_image, stb_image_write
third_party/dr_libs/   dr_wav 0.14.5, dr_mp3 0.7.3
third_party/font/      Noto Sans subset and its license
```

## Status

The library works and is tested on Linux (Wayland), both with a window and
headless. Not yet done or tested:

- Windows, macOS and high-DPI displays.
- On MSVC the library isn't yet optimized in Debug builds.
- The built-in font has Latin-1, Greek and common punctuation, but not math
  symbols such as `≤` or `∞`. Use `setFont()` with a font that has them.
- `isKeyPressed()` is covered only by the manual xdotool test
  (`tests/manual/real_input.sh`), because SDL ignores injected events for
  keyboard state.
- Audio is tested with SDL's dummy and disk drivers, which check timing and
  the mixed output, but no automated test listens to real speakers. Run
  `examples/scale` to hear it.
- Recording from a microphone (StdAudio's `startRecording`) isn't supported.
- Sound files at other sample rates are converted by linear interpolation,
  which is fine for course work but not for high-fidelity audio.

## Licenses

- This library: MIT ([`LICENSE`](LICENSE)).
- [SDL](https://libsdl.org): zlib license.
- [stb](https://github.com/nothings/stb): public domain or MIT.
- [dr_libs](https://github.com/mackron/dr_libs) (dr_wav, dr_mp3): public domain or MIT-0.
- Noto Sans: SIL Open Font License 1.1 (`third_party/font/OFL.txt`).
