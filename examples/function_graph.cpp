// Plots y = sin(4x) + sin(20x) for x from 0 to pi, by joining n sampled
// points with straight lines. With few points the plot is wrong; with
// enough it looks smooth.
//
//     ./function_graph        (200 points)
//     ./function_graph 20     (try it: too few points)
//
// After FunctionGraph in Sedgewick & Wayne, Computer Science: An
// Interdisciplinary Approach, Section 1.5. Written for this library.

#include <canvas.hpp>
#include <input.hpp>

#include <cmath>
#include <vector>

int main(int argc, char** argv) {
    const double PI = 3.14159265358979323846;
    int n = argc > 1 ? input::toInt(argv[1]) : 200;
    if (n < 1) n = 1;

    std::vector<double> x(n + 1), y(n + 1);
    for (int i = 0; i <= n; ++i) {
        x[i] = PI * i / n;
        y[i] = std::sin(4 * x[i]) + std::sin(20 * x[i]);
    }

    canvas::setTitle("y = sin(4x) + sin(20x)");
    canvas::setXscale(0, PI);
    canvas::setYscale(-2, 2);
    canvas::setPenColor(canvas::LIGHT_GRAY);
    canvas::line(0, 0, PI, 0);  // the x axis
    canvas::setPenColor(canvas::BOOK_BLUE);
    canvas::polyline(x, y);
}
