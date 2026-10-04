# canvas-cpp cheat sheet

The functions used most, on one page. Write each name with its module, as in
`canvas::circle` or `chance::uniform`. Each module's guide in [docs](.) has
the rest.

## canvas: `#include <canvas.hpp>`

Coordinates go from (0, 0) at the bottom left to (1, 1) at the top right,
unless you change them. Shapes are placed by their centre.

| Setting up | Drawing | Filled |
|---|---|---|
| `setCanvasSize(w, h)` pixels | `point(x, y)` | |
| `setScale(min, max)` both axes | `line(x0, y0, x1, y1)` | |
| `setXscale(min, max)`, `setYscale(min, max)` | `circle(x, y, r)` | `filledCircle` |
| `setPenColor(canvas::RED)`, `setPenColor(r, g, b)` | `square(x, y, half)` | `filledSquare` |
| `setPenWidth(pixels)` | `rectangle(x, y, halfW, halfH)` | `filledRectangle` |
| `setFontSize(pixels)` | `ellipse(x, y, halfW, halfH)` | `filledEllipse` |
| `setTitle("My game")` | `arc(x, y, r, from°, to°)` | |
| `clear()`, `clear(color)` | `polygon({{x, y}, ...})` | `filledPolygon` |
| | `polyline(xs, ys)` | |

**Text and pictures:** `text(x, y, "Hi")` (also numbers), `textLeft`,
`textRight`, `picture(x, y, "cat.png")`, `picture(x, y, "cat.png", w, h)`,
`picture(x, y, "ship.png", degrees)`, `save("out.png")`.

**Colours:** `BLACK`, `WHITE`, `GRAY`, `RED`, `GREEN`, `BLUE`, `YELLOW`,
`ORANGE`, ... `rgb(r, g, b)` (0–255), `gray(level)`, `hsv(hue°, s, v)`
(0–1), `mix(a, b, t)`.

**Animation:**

```cpp
canvas::enableDoubleBuffering();
canvas::setFrameRate(60);
while (true) {
    canvas::clear();
    // ...draw the next frame...
    canvas::show();            // waits for the next frame
}
```

`pause(ms)` waits. `startRecording("game.gif")` records a GIF.

**Mouse and keys** (per frame, between `show()` calls):

| | |
|---|---|
| `mouseX()`, `mouseY()` | where the mouse is |
| `mouseClicked()` | true once per click |
| `isMousePressed()` | true while held |
| `isMouseOver(x, y, halfW, halfH)` | the mouse is over a button |
| `wasKeyPressed(canvas::Key::Space)` | true once per press |
| `isKeyPressed(canvas::Key::Left)` | true while held |
| `hasNextKeyTyped()`, `nextKeyTyped()` | typed characters |
| `distance(x0, y0, x1, y1)` | between two points |

**Debugging:** `showGrid()`, `showMouseCoordinates()`, `watch("vx", vx)`,
`setDrawDelay(50)` (slow motion), `enableDebugKeys()` (P pause, N next frame,
G grid).

## turtle: `#include <turtle.hpp>`

`forward(d)`, `backward(d)`, `turnLeft(deg)`, `turnRight(deg)`, `penUp()`,
`penDown()`, `moveTo(x, y)`, `setHeading(deg)`, `home()`, `x()`, `y()`,
`heading()`. It starts in the centre, facing right, and uses the canvas pen.

## image: `#include <image.hpp>`

```cpp
image::Image img = image::load("photo.png");          // or image::create(w, h)
image::Color c = img[row][col];                         // row 0 is the top
img[row][col] = image::rgb(c.r, 0, 0);
image::save(img, "red.png");
canvas::picture(0.5, 0.5, img);                         // show it
```

`img.width`, `img.height`, `getPixel(img, row, col)`,
`setPixel(img, row, col, c)`, `crop(img, row, col, rows, cols)`.

## audio: `#include <audio.hpp>`

Samples are numbers from −1 to 1, `audio::SAMPLE_RATE` (44,100) a second.
`play(sample)`, `play(samples)`, `play("song.mp3")`, `read("a.wav")`,
`save("out.wav", samples)`, `playInBackground(...)`, `loopInBackground(...)`,
`stopInBackground()`. Sounds as samples: `tone(hz, seconds)`,
`note(semitonesFromA4, seconds)`, `silence(seconds)`.

## chance: `#include <chance.hpp>`

| | |
|---|---|
| `uniform(1, 7)` | an int from 1 to 6 |
| `uniform(n)` | an int from 0 to n − 1 |
| `uniform()`, `uniform(a, b)` | a double from 0 to 1, or a to b |
| `bernoulli(p)` | true with probability p |
| `gaussian(mean, sd)` | a normal value |
| `shuffle(v)`, `permutation(n)` | random order |
| `setSeed(42)` | the same numbers every run (or `CANVAS_SEED=42`) |

## input and output: `#include <input.hpp>`, `<output.hpp>`

| | |
|---|---|
| `int n = input::askInt("How many? ", 1, 10);` | asks until valid; also `askDouble`, `askLine`, `askYesNo` |
| `input::skipRestOfLine();` | after `std::cin >> x`, before reading a line |
| `input::getLine(std::cin, line)` | like `std::getline`, without `'\r'` |
| `input::readAllInts()`, `readAllDoubles()`, `readAllWords()`, `readAllLines()` | everything left on `std::cin` |
| `input::fromFile("data.txt");` | `std::cin` reads the file (for IDEs) |
| `output::toFile("out.txt");` | `std::cout` writes the file |
| `input::split(s)`, `split(s, ',')`, `trim(s)`, `toInt(s)`, `toDouble(s)` | taking strings apart |

## stats and stopwatch: `#include <stats.hpp>`, `<stopwatch.hpp>`

`stats::mean(v)`, `median`, `stddev`, `variance`, `min`, `max`, `sum`, for a
`std::vector<double>`. `stopwatch::start()`, then `stopwatch::elapsed()` in
seconds.

## When something goes wrong

The program stops with a message naming the function, such as
`canvas: filledCircle: radius must not be negative`. Read it first. Hints
such as `canvas: hint: circle at (200, 150) is outside the visible area`
mean the program runs but probably doesn't do what you meant.
