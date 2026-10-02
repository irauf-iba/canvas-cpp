# draw

A small 2D drawing library for introductory C++ courses, inspired by Princeton's
[StdDraw](https://introcs.cs.princeton.edu/java/stdlib/javadoc/StdDraw.html).

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
  canvas. A headless mode writes PNGs without a window, for autograding.

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
  SDL's `docs/README-linux.md` has the full list. The finished program loads
  X11 or Wayland at run time, so it runs on either.
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
| Images | `picture(x, y, file)`, `picture(x, y, file, w, h)`, `save(file)` |
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

### Mistakes stop the program with a message

A negative radius, a NaN coordinate, a missing image file and similar mistakes
print a message such as `draw: filledCircle: radius must not be negative` and
exit with status 1.

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
| `header` | `draw.hpp` compiles with strict warnings as errors; `rgb()` clamps. |
| `render.*` | Snapshot tests: each scene is compared with `tests/reference/<scene>.png`, allowing small differences. On failure, `<scene>-diff.png` in the build's `tests` folder marks the differing pixels in red. |
| `input` | Clicks and typed keys (injected as SDL events), per-frame input rules, `pause()` timing. |
| `window.*` | The window stays open after `main()` returns, and closing it ends the program, including in the middle of drawing. |
| `error.*` | Each mistake stops the program with the expected message. |

After an intended change to rendering, regenerate the reference images, check
them by eye, and commit them:

```sh
DRAW_UPDATE_REFERENCES=1 ctest --test-dir build -R render
```

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
  Sans. `picture()` and `save()` use stb_image and stb_image_write.
- **Speed.** The library is compiled with optimization even when the student's
  project builds in Debug.

```
include/draw.hpp       the public API
src/draw.cpp           implementation
src/font_data.inc      built-in font (generated by tools/make_font.sh)
examples/              shapes, bouncing_ball, sketch
tests/                 tests, reference images and benchmark
third_party/stb/       stb_truetype, stb_image, stb_image_write
third_party/font/      Noto Sans subset and its license
```

## Status

The library works and is tested on Linux (Wayland), both with a window and
headless. Not yet done or tested:

- Windows, macOS and high-DPI displays.
- On MSVC the library isn't yet optimized in Debug builds.
- The built-in font has Latin-1, Greek and common punctuation, but not math
  symbols such as `≤` or `∞`. Use `setFont()` with a font that has them.
- `isKeyPressed()` has no automated test, because SDL ignores injected events
  for keyboard state.

Planned: `audio` and `image` modules alongside `draw`.

## Licenses

- This library: MIT ([`LICENSE`](LICENSE)).
- [SDL](https://libsdl.org): zlib license.
- [stb](https://github.com/nothings/stb): public domain or MIT.
- Noto Sans: SIL Open Font License 1.1 (`third_party/font/OFL.txt`).
