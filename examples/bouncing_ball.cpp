// A ball bouncing inside the window. Shows the double-buffered animation loop.

#include <cmath>
#include <draw.hpp>

int main() {
    draw::setScale(-1.0, 1.0);
    draw::enableDoubleBuffering();

    double x = 0.48, y = 0.86;      // position
    double vx = 0.015, vy = 0.023;  // velocity per frame
    const double radius = 0.05;

    while (true) {
        if (std::abs(x + vx) > 1.0 - radius) vx = -vx;
        if (std::abs(y + vy) > 1.0 - radius) vy = -vy;
        x += vx;
        y += vy;

        draw::clear(draw::LIGHT_GRAY);
        draw::setPenColor(draw::BLACK);
        draw::filledCircle(x, y, radius);
        draw::show();
        draw::pause(16);
    }
}
