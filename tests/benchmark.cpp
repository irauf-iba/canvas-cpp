// Rough timings of common operations. Not run by ctest.
//
// Run headless to measure drawing alone:  DRAW_HEADLESS=1 ./benchmark
// Run with a window to also measure animation frame pacing:  ./benchmark

#include <draw.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

double msSince(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

template <typename F>
void time(const char* label, F f) {
    auto start = Clock::now();
    f();
    std::printf("%-40s %8.2f ms\n", label, msSince(start));
}

void framePacing() {
    // 1000 moving balls per frame, double buffered, pause(16).
    draw::enableDoubleBuffering();
    std::vector<double> frames;
    auto last = Clock::now();
    for (int f = 0; f < 130; ++f) {
        draw::clear();
        for (int i = 0; i < 1000; ++i) {
            double t = f * 0.02 + i;
            draw::setPenColor(draw::rgb(i % 255, 100, 200));
            draw::filledCircle(0.5 + 0.4 * std::cos(t), 0.5 + 0.4 * std::sin(1.3 * t), 0.01);
        }
        draw::show();
        draw::pause(16);
        frames.push_back(msSince(last));
        last = Clock::now();
    }
    frames.erase(frames.begin(), frames.begin() + 10);  // warm-up
    std::sort(frames.begin(), frames.end());
    double sum = 0;
    for (double f : frames) sum += f;
    std::printf("animation, 1000 balls, pause(16): mean %.2f ms, median %.2f, p95 %.2f, max %.2f\n",
                sum / static_cast<double>(frames.size()), frames[frames.size() / 2],
                frames[frames.size() * 95 / 100], frames.back());
    draw::disableDoubleBuffering();
}

}  // namespace

int main() {
    draw::point(0, 0);  // open the window (or headless canvas) before timing

    time("1000 small filled circles", [] {
        for (int i = 0; i < 1000; ++i) draw::filledCircle(0.5, 0.5, 0.01);
    });
    time("1000 large filled circles (r = 0.2)", [] {
        for (int i = 0; i < 1000; ++i) draw::filledCircle(0.5, 0.5, 0.2);
    });
    time("1000 outline circles (r = 0.2)", [] {
        for (int i = 0; i < 1000; ++i) draw::circle(0.5, 0.5, 0.2);
    });
    time("1000 corner-to-corner lines", [] {
        for (int i = 0; i < 1000; ++i) draw::line(0, 0, 1, 1);
    });
    time("1000 text strings", [] {
        for (int i = 0; i < 1000; ++i) draw::text(0.5, 0.5, "Hello, world 123");
    });
    time("100 clears", [] {
        for (int i = 0; i < 100; ++i) draw::clear();
    });
    time("20000 points (immediate mode)", [] {
        for (int i = 0; i < 20000; ++i) draw::point(i / 20000.0, 0.5);
    });

    const char* headless = std::getenv("DRAW_HEADLESS");
    if (!(headless && *headless && *headless != '0')) framePacing();
    std::fflush(stdout);
    std::_Exit(0);  // don't keep the window open
}
