// A turtle spiral: the turtle draws a polygon, but each side is a little
// shorter than the one before, so it spirals inwards.
//
//     ./spiral            (6 sides per turn, each side 3% shorter)
//     ./spiral 3 0.95
//     ./spiral 60 0.995   (nearly a smooth curve)
//
// After Spiral in Sedgewick & Wayne, Computer Science: An Interdisciplinary
// Approach, Section 3.2. Written for this library.

#include <canvas.hpp>
#include <input.hpp>
#include <turtle.hpp>

#include <cmath>

int main(int argc, char** argv) {
    int sides = argc > 1 ? input::toInt(argv[1]) : 6;
    double shrink = argc > 2 ? input::toDouble(argv[2]) : 0.97;
    if (sides < 3) sides = 3;

    // The first polygon fits in a circle of radius 0.4 around the centre: its
    // sides are 2 * 0.4 * sin(180 / sides degrees) long, and its bottom side
    // starts below and to the left of the centre.
    const double PI = 3.14159265358979323846;
    double step = 2 * 0.4 * std::sin(PI / sides);
    double apothem = 0.4 * std::cos(PI / sides);

    canvas::setTitle("Spiral");
    canvas::setPenColor(canvas::BOOK_BLUE);
    turtle::penUp();
    turtle::moveTo(0.5 - step / 2, 0.5 - apothem);
    turtle::penDown();

    while (step > 0.002) {
        turtle::forward(step);
        turtle::turnLeft(360.0 / sides);
        step *= shrink;
    }
}
