// Reads a bounding box and then points from standard input, and plots the
// points. The first line holds xmin ymin xmax ymax; each line after it holds
// one point, x y.
//
//     ./plot_filter < examples/textbook/data/cities.txt
//
// cities.txt holds the longitude and latitude of 1251 cities (from Natural
// Earth, public domain), so the plot is a map of the world.
//
// After PlotFilter in Sedgewick & Wayne, Computer Science: An
// Interdisciplinary Approach, Section 1.5. Written for this library.

#include <canvas.hpp>

#include <iostream>

int main() {
    double xmin, ymin, xmax, ymax;
    if (!(std::cin >> xmin >> ymin >> xmax >> ymax)) {
        std::cerr << "expected a bounding box: xmin ymin xmax ymax\n";
        return 1;
    }

    canvas::setTitle("Plot filter");
    canvas::setCanvasSize(1024, 512);
    canvas::setXscale(xmin, xmax);
    canvas::setYscale(ymin, ymax);
    canvas::setPenColor(canvas::BOOK_BLUE);
    canvas::setPenWidth(3);

    double x, y;
    int count = 0;
    while (std::cin >> x >> y) {
        canvas::point(x, y);
        ++count;
    }
    std::cout << count << " points\n";
}
