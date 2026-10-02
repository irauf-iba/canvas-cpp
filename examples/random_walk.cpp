// A random walk: a point takes 5000 random steps on a grid, leaving a trail.
// Prints the seed, so a walk you like can be repeated with
// CANVAS_SEED=<seed> ./random_walk.

#include <canvas.hpp>
#include <chance.hpp>

#include <cstdio>

int main() {
    std::printf("seed %llu\n", static_cast<unsigned long long>(chance::seed()));

    const int n = 60;  // grid size
    canvas::setTitle("Random walk");
    canvas::setScale(-0.5, n - 0.5);
    canvas::enableDoubleBuffering();
    canvas::setPenWidth(3);

    int x = n / 2, y = n / 2;
    for (int step = 0; step < 5000 && x >= 0 && x < n && y >= 0 && y < n; ++step) {
        int oldX = x, oldY = y;
        int direction = chance::uniform(4);  // 0 to 3
        if (direction == 0) ++x;
        if (direction == 1) --x;
        if (direction == 2) ++y;
        if (direction == 3) --y;

        canvas::setPenColor(canvas::rgb(step % 256, 80, 255 - step % 256));
        canvas::line(oldX, oldY, x, y);
        if (step % 10 == 0) {
            canvas::show();
            canvas::pause(16);
        }
    }
    canvas::show();
}
