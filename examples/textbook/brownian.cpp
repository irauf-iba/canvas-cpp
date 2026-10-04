// A Brownian bridge, drawn by midpoint displacement: the midpoint of a line
// is moved up or down by a random (Gaussian) amount, and then each half is
// treated the same way, with smaller moves. The result looks like the
// outline of a mountain range, or a stock price over time.
//
//     ./brownian          (Hurst exponent 0.5)
//     ./brownian 0.8      (smoother; 0.2 is rougher)
//
// Prints the seed, so a picture you like can be drawn again with
// CANVAS_SEED=<seed> ./brownian.
//
// After Brownian in Sedgewick & Wayne, Computer Science: An Interdisciplinary
// Approach, Section 2.3. Written for this library.

#include <canvas.hpp>
#include <chance.hpp>
#include <input.hpp>

#include <cmath>
#include <iostream>

// Draws a Brownian bridge from (x0, y0) to (x1, y1). Each level of recursion
// divides the variance of the move by `shrink`.
void curve(double x0, double y0, double x1, double y1, double variance, double shrink) {
    if (x1 - x0 < 0.002) {
        canvas::line(x0, y0, x1, y1);
        return;
    }
    double xm = (x0 + x1) / 2;
    double ym = (y0 + y1) / 2 + chance::gaussian(0, std::sqrt(variance));
    curve(x0, y0, xm, ym, variance / shrink, shrink);
    curve(xm, ym, x1, y1, variance / shrink, shrink);
}

int main(int argc, char** argv) {
    double hurst = argc > 1 ? input::toDouble(argv[1]) : 0.5;
    std::cout << "seed " << chance::seed() << "\n";

    canvas::setTitle("Brownian bridge");
    canvas::setPenColor(canvas::BOOK_BLUE);
    canvas::setPenWidth(1.5);
    curve(0, 0.5, 1, 0.5, 0.01, std::pow(2, 2 * hurst));
}
