// In headless mode (CANVAS_HEADLESS=1) nothing waits and no input is
// reported, so an autograder can run animation and game loops quickly.

#include <canvas.hpp>

#include <chrono>
#include <cstdio>

#include "check.hpp"

int main() {
    auto start = std::chrono::steady_clock::now();
    canvas::enableDoubleBuffering();
    canvas::setFrameRate(1);  // one frame per second, if it waited
    canvas::setDrawDelay(500);  // half a second per drawing call, if it waited
    for (int i = 0; i < 5; ++i) {
        canvas::clear();
        canvas::filledCircle(0.5, 0.5, 0.1 * i);
        canvas::show();
        canvas::pause(1000);
    }
    double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    std::printf("5 frames with setFrameRate(1), setDrawDelay(500) and pause(1000) took %.1f ms\n", ms);
    CHECK(ms < 500);

    CHECK(!canvas::mouseClicked());
    CHECK(!canvas::isMousePressed());
    CHECK(!canvas::hasNextKeyTyped());
    CHECK(!canvas::isKeyPressed(canvas::Key::Space));
    CHECK(!canvas::wasKeyPressed(canvas::Key::Space));
    return finish();
}
