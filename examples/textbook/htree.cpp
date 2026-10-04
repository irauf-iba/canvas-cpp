// An H-tree of order n: the letter H, with an H-tree of order n - 1, half
// the size, centred on each of its four tips.
//
//     ./htree      (order 5)
//     ./htree 7
//
// Lines get thinner at each level, so the big H stays easy to see.
//
// After Htree in Sedgewick & Wayne, Computer Science: An Interdisciplinary
// Approach, Section 2.3. Written for this library.

#include <canvas.hpp>
#include <input.hpp>

// Draws an H-tree of the given order, centred at (x, y), size wide.
void htree(int order, double x, double y, double size) {
    if (order == 0) return;
    canvas::setPenWidth(order);
    double x0 = x - size / 2, x1 = x + size / 2;
    double y0 = y - size / 2, y1 = y + size / 2;
    canvas::line(x0, y, x1, y);    // the crossbar
    canvas::line(x0, y0, x0, y1);  // the left leg
    canvas::line(x1, y0, x1, y1);  // the right leg
    htree(order - 1, x0, y0, size / 2);
    htree(order - 1, x0, y1, size / 2);
    htree(order - 1, x1, y0, size / 2);
    htree(order - 1, x1, y1, size / 2);
}

int main(int argc, char** argv) {
    int order = argc > 1 ? input::toInt(argv[1]) : 5;
    canvas::setTitle("H-tree");
    canvas::setPenColor(canvas::BOOK_RED);
    // canvas::setDrawDelay(30);   // uncomment to watch the recursion draw it, line by line
    htree(order, 0.5, 0.5, 0.5);
}
