# turtle: drawing by moving and turning

`#include <turtle.hpp>`

The `turtle` module is a C++ version of Princeton's
[Turtle](https://introcs.cs.princeton.edu/java/stdlib/javadoc/Turtle.html),
from the Logo tradition. Imagine a turtle on the canvas holding a pen: it moves
forward and turns, and draws a line wherever it goes while its pen is down.
This makes loops and, above all, recursion visible.

```cpp
#include <turtle.hpp>

int main() {
    for (int i = 0; i < 5; ++i) {   // a five-pointed star
        turtle::forward(0.4);
        turtle::turnRight(144);
    }
}
```

The header [`include/turtle.hpp`](../include/turtle.hpp) documents every
function.

## Where the turtle is

The turtle starts at the centre of the canvas, facing right, with its pen
down. Headings are in degrees, counterclockwise from the positive x axis, as
in `canvas::arc`:

| Heading | Faces |
|---|---|
| 0 | right |
| 90 | up |
| 180 | left |
| 270 | down |

There is one turtle, like there is one canvas.

## Functions

| Function | Does |
|---|---|
| `turtle::forward(step)`, `turtle::backward(step)` | Moves along the heading, or the opposite way, drawing a line if the pen is down. |
| `turtle::turnLeft(degrees)`, `turtle::turnRight(degrees)` | Turns on the spot, counterclockwise or clockwise. |
| `turtle::penUp()`, `turtle::penDown()` | Lifts or lowers the pen. With the pen up, the turtle moves without drawing. |
| `turtle::moveTo(x, y)` | Moves straight to (x, y), drawing a line if the pen is down; the heading stays the same. |
| `turtle::setHeading(degrees)` | Turns to face a heading. |
| `turtle::x()`, `turtle::y()`, `turtle::heading()` | Where the turtle is, and which way it faces (from 0 up to, but not including, 360). |
| `turtle::home()` | Back to the centre of the canvas, facing right. It doesn't draw, and leaves the pen up or down as it was. |

To start somewhere else without drawing a line there, lift the pen first:

```cpp
turtle::penUp();
turtle::moveTo(0.2, 0.1);
turtle::setHeading(90);
turtle::penDown();
```

## The turtle draws on the canvas

The turtle draws with the canvas pen, in canvas coordinates, so everything
from [canvas](canvas.md) works with it:

- `canvas::setPenColor` and `canvas::setPenWidth` change the turtle's lines.
- Distances are in canvas coordinates: with the default scale, `forward(0.5)`
  crosses half the canvas, and `canvas::setScale(-10, 10)` makes `forward(1)`
  a twentieth of the width. The turtle starts at the centre of whatever scale
  is set when it is first used.
- Turtle drawing mixes with `canvas::circle`, `canvas::text` and the rest, and
  with animation: `canvas::enableDoubleBuffering()`, `canvas::show()` and
  `canvas::setFrameRate()`.

Including `turtle.hpp` also includes `canvas.hpp`.

Use the same scale for x and y (`canvas::setScale`) for turtle drawings: with
different scales, a 90° turn doesn't look like a right angle.

## Recursion with the turtle

Recursive drawings are the turtle's best use. The Koch curve replaces a line
with four lines a third as long:

```cpp
void koch(int depth, double size) {
    if (depth == 0) {
        turtle::forward(size);
        return;
    }
    koch(depth - 1, size / 3);
    turtle::turnLeft(60);
    koch(depth - 1, size / 3);
    turtle::turnRight(120);
    koch(depth - 1, size / 3);
    turtle::turnLeft(60);
    koch(depth - 1, size / 3);
}
```

For branching drawings such as trees, have each call leave the turtle where it
started, facing the same way. Then every branch can rely on that:

```cpp
void tree(int depth, double length) {
    if (depth == 0) return;
    turtle::forward(length);
    turtle::turnLeft(25);
    tree(depth - 1, length * 0.7);
    turtle::turnRight(50);
    tree(depth - 1, length * 0.7);
    turtle::turnLeft(25);
    turtle::penUp();               // back to where this call started
    turtle::backward(length);
    turtle::penDown();
}
```

## Errors

Mistakes stop the program with a message and exit status 1, for example:

```
turtle: forward: an argument is NaN or infinite
```

## Differences from Princeton's Turtle

| Princeton | turtle | Why |
|---|---|---|
| `Turtle t = new Turtle(x0, y0, a0)` | One turtle, `turtle::…` functions | No objects; one turtle, like one canvas. Use `moveTo` and `setHeading` to start elsewhere. |
| `t.goForward(step)` | `turtle::forward(step)` | Shorter, as in Logo and Python. |
| `t.turnLeft(delta)` | `turtle::turnLeft(degrees)`, and also `turnRight` | Both directions read naturally. |
| — | `penUp`, `penDown`, `moveTo`, `backward`, `home`, `x`, `y`, `heading` | For moving without drawing, and for recursion that returns the turtle to where it started. |
| `t.setPenColor(c)` | `canvas::setPenColor(c)` | The turtle uses the canvas pen. |

## Examples

- [`examples/koch.cpp`](../examples/koch.cpp): a Koch snowflake, asking for
  the depth with `input::askInt`.
- [`examples/tree.cpp`](../examples/tree.cpp): a recursive tree, with branches
  getting thinner and turning green.
