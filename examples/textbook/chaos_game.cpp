// The chaos game: start at a corner of a triangle, then over and over jump
// halfway towards a randomly chosen corner and draw a dot there. Out of
// randomness, the Sierpinski triangle appears.
//
//     ./chaos_game          (30000 dots)
//     ./chaos_game 100000
//
// After the Sierpinski exercise in Sedgewick & Wayne, Computer Science: An
// Interdisciplinary Approach, Section 2.2. Written for this library.

#include <canvas.hpp>
#include <chance.hpp>
#include <input.hpp>

#include <cmath>

int main(int argc, char** argv) {
    int dots = argc > 1 ? input::toInt(argv[1]) : 30000;

    const double cx[] = {0.05, 0.95, 0.5};
    const double cy[] = {0.1, 0.1, 0.1 + 0.9 * std::sqrt(3.0) / 2};
    const canvas::Color colors[] = {canvas::BOOK_RED, canvas::BOOK_BLUE, canvas::rgb(40, 150, 60)};

    canvas::setTitle("Chaos game");
    canvas::enableDoubleBuffering();
    canvas::setFrameRate(60);
    canvas::setPenWidth(1.5);

    double x = cx[0], y = cy[0];
    for (int i = 0; i < dots; ++i) {
        int corner = chance::uniform(3);
        x = (x + cx[corner]) / 2;
        y = (y + cy[corner]) / 2;
        canvas::setPenColor(colors[corner]);
        canvas::point(x, y);
        if (i % 500 == 0) canvas::show();  // watch the triangle appear
    }
    canvas::show();
}
