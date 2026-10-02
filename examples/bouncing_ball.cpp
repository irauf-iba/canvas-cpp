// A ball bouncing inside the window. Shows the double-buffered animation loop.

#include <cmath>
#include <canvas.hpp>

int main() {
    canvas::setScale(-1.0, 1.0);
    canvas::enableDoubleBuffering();

    double x = 0.48, y = 0.86;      // position
    double vx = 0.015, vy = 0.023;  // velocity per frame
    const double radius = 0.05;

    while (true) {
        if (std::abs(x + vx) > 1.0 - radius) vx = -vx;
        if (std::abs(y + vy) > 1.0 - radius) vy = -vy;
        x += vx;
        y += vy;

        canvas::clear(canvas::LIGHT_GRAY);
        canvas::setPenColor(canvas::BLACK);
        canvas::filledCircle(x, y, radius);
        canvas::show();
        canvas::pause(16);
    }
}
