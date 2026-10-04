// turtle.hpp - turtle graphics on the canvas, inspired by Princeton's Turtle.
//
// The turtle sits on the canvas holding a pen. It moves forward and turns,
// drawing a line wherever it goes while the pen is down:
//
//     #include <turtle.hpp>
//
//     int main() {
//         for (int i = 0; i < 4; ++i) {   // a square
//             turtle::forward(0.3);
//             turtle::turnLeft(90);
//         }
//     }
//
// It starts at the centre of the canvas, facing right (heading 0), with the
// pen down. Headings are in degrees, counterclockwise from the positive x
// axis, as in canvas::arc: 90 is up, 180 is left, 270 is down.
//
// The turtle draws on the canvas with the canvas pen, so canvas::setPenColor
// and canvas::setPenWidth change its lines, distances are in canvas
// coordinates, and turtle drawing can be mixed with any other canvas drawing.
// (With different x and y scales, turns look stretched; use canvas::setScale
// for turtle drawings.)
//
// Errors, such as a step that is NaN, print a message and stop the program.

#ifndef CANVAS_TURTLE_HPP
#define CANVAS_TURTLE_HPP

#include "canvas.hpp"  // so canvas::setPenColor and the rest work with this header alone

namespace turtle {

// Moves the given distance along the heading (backward: the opposite way),
// drawing a line if the pen is down.
void forward(double step);
void backward(double step);

// Turns on the spot, counterclockwise (left) or clockwise (right).
void turnLeft(double degrees);
void turnRight(double degrees);

// Lifts or lowers the pen. While it is up, the turtle moves without drawing.
void penUp();
void penDown();

// Moves straight to (x, y), drawing a line if the pen is down. The heading
// doesn't change.
void moveTo(double x, double y);

// Turns to face the given heading.
void setHeading(double degrees);

// Where the turtle is, and which way it faces (0 up to but not including 360).
double x();
double y();
double heading();

// Back to the start: the centre of the canvas, facing right. Doesn't draw,
// and doesn't change the pen.
void home();

}  // namespace turtle

#endif  // CANVAS_TURTLE_HPP
