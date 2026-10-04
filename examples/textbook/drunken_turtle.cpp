// A drunken turtle: before each small step it turns to a random direction.
// Watch where it wanders. Prints the seed, so a walk can be repeated with
// CANVAS_SEED=<seed> ./drunken_turtle.
//
//     ./drunken_turtle         (3000 steps)
//     ./drunken_turtle 10000
//
// After DrunkenTurtle in Sedgewick & Wayne, Computer Science: An
// Interdisciplinary Approach, Section 3.2. Written for this library.

#include <canvas.hpp>
#include <chance.hpp>
#include <input.hpp>
#include <turtle.hpp>

#include <iostream>

int main(int argc, char** argv) {
    int steps = argc > 1 ? input::toInt(argv[1]) : 3000;
    std::cout << "seed " << chance::seed() << "\n";

    canvas::setTitle("Drunken turtle");
    canvas::enableDoubleBuffering();
    canvas::setFrameRate(60);
    canvas::setPenWidth(1.5);

    for (int i = 0; i < steps; ++i) {
        canvas::setPenColor(canvas::rgb(255 * i / steps, 60, 255 - 255 * i / steps));
        turtle::setHeading(chance::uniform(0.0, 360.0));
        turtle::forward(0.005);
        if (i % 10 == 0) canvas::show();  // 10 steps per frame
    }
    canvas::show();
}
