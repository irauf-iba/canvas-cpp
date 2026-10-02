# draw: drawing, animation and input

`#include <draw.hpp>`

The `draw` module is a C++ version of Princeton's
[StdDraw](https://introcs.cs.princeton.edu/java/stdlib/javadoc/StdDraw.html).
Everything is a free function in `namespace draw`. The window opens on the
first drawing call and stays open after `main()` returns, until the user closes
it. Closing the window ends the program.

```cpp
#include <draw.hpp>

int main() {
    draw::setPenColor(draw::BOOK_BLUE);
    draw::filledCircle(0.5, 0.5, 0.25);
    draw::setPenColor(draw::BLACK);
    draw::text(0.5, 0.1, "Hello, draw!");
}
```

The header [`include/draw.hpp`](../include/draw.hpp) documents every function.

## Coordinates

By default the canvas is 512 × 512 pixels, with (0, 0) at the lower left and
(1, 1) at the upper right; y points up. Change the ranges with `setXscale`,
`setYscale` or `setScale`.

Pen widths and font sizes are in pixels, so they don't change when the scale
does.

Circles, squares and arcs are always round and square, even on a rectangular
canvas or when the x and y scales differ, as in a plot. Their radius or
half-length is measured in x units and used in both directions. `ellipse` and
`rectangle` take a separate half-width and half-height, and follow each axis's
scale.

## Functions

### Window and coordinates

| Function | Does |
|---|---|
| `setCanvasSize(width, height)` | Canvas size in pixels (default 512 × 512). Clears the canvas. |
| `setTitle(title)` | Window title (default `"draw"`). |
| `setXscale(min, max)`, `setYscale(min, max)` | Range of x or y shown (default 0 to 1). |
| `setScale(min, max)` | Sets both ranges. |

### Pen, colors and text

| Function | Does |
|---|---|
| `setPenColor(color)`, `setPenColor(r, g, b)`, `penColor()` | Color for everything drawn (default `BLACK`). |
| `setPenWidth(pixels)`, `penWidth()` | Width of lines, outlines and points (default 2). |
| `setFontSize(pixels)` | Text height (default 16). |
| `setFont(ttfFile)` | Use a TrueType font file instead of the built-in one. |

**Colors.** `draw::Color` has `r`, `g`, `b` and `a` (opacity) components from 0
to 255. Use `draw::rgb(r, g, b)` or `draw::rgb(r, g, b, a)` to make one from
`int` values: it clamps to 0–255, and unlike `Color{r, g, b}` it accepts `int`
variables. Colors can be compared with `==`. The predefined colors are:
`BLACK`, `WHITE`, `GRAY`, `LIGHT_GRAY`, `DARK_GRAY`, `RED`, `GREEN`, `BLUE`,
`CYAN`, `MAGENTA`, `YELLOW`, `ORANGE`, `PINK`, `BROWN`, `PURPLE`, and the
textbook's `BOOK_BLUE`, `BOOK_LIGHT_BLUE` and `BOOK_RED`. The `image` module
uses the same colors.

### Shapes

Shapes are positioned by their center. Outline shapes use the pen width, and
filled shapes are filled with the pen color.

| Outline | Filled | Size |
|---|---|---|
| `point(x, y)` | | a dot as wide as the pen |
| `line(x0, y0, x1, y1)` | | |
| `circle(x, y, radius)` | `filledCircle` | |
| `ellipse(x, y, halfWidth, halfHeight)` | `filledEllipse` | |
| `arc(x, y, radius, angle1, angle2)` | | degrees, counterclockwise from the x axis |
| `square(x, y, halfLength)` | `filledSquare` | |
| `rectangle(x, y, halfWidth, halfHeight)` | `filledRectangle` | |
| `polygon(points)` | `filledPolygon(points)` | closed |
| `polyline(points)` | | open, e.g. to plot a function |

`polygon`, `filledPolygon` and `polyline` take a list of `draw::Point`, or the
x and y coordinates as two vectors:

```cpp
draw::filledPolygon({{0.1, 0.1}, {0.5, 0.9}, {0.9, 0.1}});
draw::polyline(xs, ys);   // std::vector<double> xs, ys of equal size
```

### Text

| Function | Does |
|---|---|
| `text(x, y, s)` | Text centered at (x, y). |
| `textLeft(x, y, s)`, `textRight(x, y, s)` | Text starting or ending at (x, y). |
| `text(x, y, s, degrees)` | Centered text, rotated counterclockwise. |

The built-in font is a subset of Noto Sans with Latin-1, Greek and common
punctuation. It doesn't have math symbols such as `≤` or `∞`; use `setFont`
with a font that has them.

### Pictures and the canvas

| Function | Does |
|---|---|
| `picture(x, y, file)` | Draws a .png, .jpg, .bmp or .gif file centered at (x, y), at its natural size. |
| `picture(x, y, file, width, height)` | The same, scaled to a width and height in user coordinates. |
| `picture(x, y, img)`, `picture(x, y, img, width, height)` | The same for an `image::Image` (see [image.md](image.md)). At natural size each image pixel covers exactly one canvas pixel. |
| `save(file)` | Saves the canvas as .png, .jpg or .bmp, at the size set by `setCanvasSize`. |
| `canvas()` | A copy of the canvas as an `image::Image`, to read or process its pixels. |

### Animation

| Function | Does |
|---|---|
| `clear()`, `clear(color)` | Fills the canvas (default `WHITE`). |
| `enableDoubleBuffering()`, `disableDoubleBuffering()` | Draw off screen until `show()`. |
| `show()` | Copies the canvas to the screen. |
| `pause(ms)` | Waits. |

Normally each drawing call appears on screen right away. For smooth animation,
enable double buffering so that each frame appears all at once:

```cpp
draw::enableDoubleBuffering();
while (true) {
    draw::clear();
    // ...draw the next frame...
    draw::show();
    draw::pause(16);   // about 60 frames per second
}
```

In an animation loop the time spent drawing counts toward `pause()`, so frames
stay evenly spaced even when drawing takes a while. A one-off `pause(2000)`
still waits two full seconds.

### Mouse and keyboard

| Function | Does |
|---|---|
| `mouseX()`, `mouseY()` | Mouse position in user coordinates. |
| `isMousePressed()` | True while a mouse button is held down. |
| `mouseClicked()` | True once for each click in the current frame. |
| `hasNextKeyTyped()`, `nextKeyTyped()` | Characters typed in the current frame, oldest first. |
| `isKeyPressed(key)` | True while the key is held down, e.g. `draw::Key::Left`. |

`draw::Key` has `A` to `Z`, `Num0` to `Num9`, `Space`, `Enter`, `Escape`,
`Backspace`, `Tab`, `Left`, `Right`, `Up`, `Down`, `Shift`, `Control` and
`Alt`.

**Input happens per frame.** `show()` and `pause()` end a frame. Clicks and
typed characters that the program didn't read during a frame are dropped when
the next frame starts, so a program that falls behind never reacts to old
input.

- `mouseClicked()` reports each click once. Clicks from earlier frames are
  forgotten.
- Up to 16 unread typed characters are kept; more, e.g. from pasting, are
  ignored. Only ASCII characters are reported. Enter is `'\n'`, Backspace
  `'\b'`, Tab `'\t'`, Escape 27 and Delete 127. Use `if` to handle one key per
  frame, or `while` to handle every key:

  ```cpp
  while (draw::hasNextKeyTyped()) {
      char c = draw::nextKeyTyped();
      // ...
  }
  ```

- For game controls, use `isKeyPressed` and `isMousePressed`, which report
  what is held down right now.

Input queries are cheap, about 50 ns, so a program can check the keyboard very
often, for example once per audio sample.

## Headless mode

If the environment variable `DRAW_HEADLESS` is set to 1, no window is opened.
Drawing works as usual, `pause()` returns immediately, no input is ever
reported, and `save()` writes the canvas. Student code needs no changes, which
makes this useful for autograding:

```sh
DRAW_HEADLESS=1 ./student_program && compare out.png expected.png
```

`save()` always writes the size set by `setCanvasSize`, even on high-DPI
screens, so output is the same on every machine.

## Errors

Mistakes stop the program with a message and exit status 1, for example:

```
draw: filledCircle: radius must not be negative
draw: circle: an argument is NaN or infinite
draw: picture: cannot open 'cat.png' (can't fopen)
draw: polygon: x and y must have the same size
draw: nextKeyTyped: no key was typed (check hasNextKeyTyped() first)
```

## Differences from Java's StdDraw

| StdDraw | draw | Why |
|---|---|---|
| `setPenRadius(0.002)` | `setPenWidth(2)` | Pixels are easier to reason about, and widths don't change with the scale. |
| `getPenColor()` | `penColor()` | Shorter and C++-style. |
| `new Color(r, g, b)` | `draw::rgb(r, g, b)` | Accepts `int` variables and clamps to 0–255. |
| `double[] x, double[] y` | `{{x, y}, ...}` or two vectors | Point lists read naturally, e.g. `polygon({{0, 0}, {1, 0}, {0.5, 1}})`. |
| — | `polyline(points)` | For plotting functions. |
| Polling `isMousePressed()` for clicks | `mouseClicked()` | Short clicks between two polls aren't missed. |
| Typed keys queue forever | Dropped at the end of each frame | A lagging program doesn't replay old keys. |
| Circles and squares stretch when the x and y scales differ | Always round and square; size in x units | A circle should look like a circle, and plot markers stay dots. |
| `pause(ms)` sleeps | `pause(ms)` keeps a steady frame rate | Smooth animation even when drawing takes time. |
| Exceptions | Message and exit | Clearer for beginners than an uncaught exception. |

## How it works

- **Drawing.** Each call rasterizes straight into an RGBA canvas in memory, on
  the caller's thread. There is no render thread and no list of shapes.
- **Filled shapes** use exact-area coverage, so edges are antialiased and
  self-crossing polygons fill by the nonzero rule.
- **Lines and outlines** take their coverage from the distance to the line
  segments. That gives round joins and caps, and overlapping pieces join
  without visible seams.
- **Displaying.** SDL copies the canvas to a window texture: on `show()` with
  vsync in double-buffered mode, and otherwise automatically, at most about 60
  times a second. On high-DPI screens the canvas has more pixels than its
  logical size, so drawing stays sharp.
- **Text** uses stb_truetype, with the built-in font embedded in the library.
- **Speed.** The library is compiled with optimization even when the student's
  project builds in Debug. A thousand small filled circles take about 1.5 ms.

## Examples

- [`examples/shapes.cpp`](../examples/shapes.cpp): one of each kind of shape.
- [`examples/bouncing_ball.cpp`](../examples/bouncing_ball.cpp): a
  double-buffered animation loop.
- [`examples/sketch.cpp`](../examples/sketch.cpp): drawing with the mouse,
  keys to change color, clear and save.
