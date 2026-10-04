// Reads a bounding box and then points from standard input, and plots the
// points. The first line holds xmin ymin xmax ymax; each line after it holds
// one point, x y.
//
//     ./plot_filter < examples/textbook/data/cities.txt
//     ./plot_filter < examples/textbook/data/pakistan_cities.txt
//
// The points in these files are the longitude and latitude of cities, so the
// plots are maps: cities.txt has 1251 cities of the world (from Natural
// Earth, public domain), and pakistan_cities.txt has 570 places in Pakistan
// with at least 1000 people (from GeoNames, geonames.org, CC BY 4.0).
//
// The window gets the proportions of the bounding box.
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

    // 800 pixels across, and as tall as the bounding box's proportions need.
    int width = 800;
    int height = static_cast<int>(width * (ymax - ymin) / (xmax - xmin));
    if (height < 200) height = 200;
    if (height > 900) height = 900;

    canvas::setTitle("Plot filter");
    canvas::setCanvasSize(width, height);
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
