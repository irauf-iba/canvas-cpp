// Rough timings of common operations. Not run by ctest.
//
// Run headless to measure drawing alone:  CANVAS_HEADLESS=1 ./benchmark
// Run with a window to also measure animation frame pacing, with pause(16)
// and with setFrameRate(60):  ./benchmark

#include <canvas.hpp>

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

// 1000 moving balls per frame, double buffered, paced either by pause(16) or
// by setFrameRate(60).
void framePacing(bool useFrameRate) {
    canvas::enableDoubleBuffering();
    canvas::setFrameRate(useFrameRate ? 60 : 0);
    std::vector<double> frames;
    auto last = Clock::now();
    for (int f = 0; f < 190; ++f) {
        canvas::clear();
        for (int i = 0; i < 1000; ++i) {
            double t = f * 0.02 + i;
            canvas::setPenColor(canvas::rgb(i % 255, 100, 200));
            canvas::filledCircle(0.5 + 0.4 * std::cos(t), 0.5 + 0.4 * std::sin(1.3 * t), 0.01);
        }
        canvas::show();
        if (!useFrameRate) canvas::pause(16);
        frames.push_back(msSince(last));
        last = Clock::now();
    }
    frames.erase(frames.begin(), frames.begin() + 10);  // warm-up
    std::sort(frames.begin(), frames.end());
    double sum = 0;
    int long_frames = 0;
    for (double f : frames) {
        sum += f;
        if (f > 25) ++long_frames;
    }
    std::printf("animation, 1000 balls, %-16s mean %.2f ms, median %.2f, p95 %.2f, max %.2f, frames > 25 ms: %d\n",
                useFrameRate ? "setFrameRate(60):" : "pause(16):", sum / static_cast<double>(frames.size()),
                frames[frames.size() / 2], frames[frames.size() * 95 / 100], frames.back(), long_frames);
    canvas::setFrameRate(0);
    canvas::disableDoubleBuffering();
}

}  // namespace

int main() {
    canvas::point(0, 0);  // open the window (or headless canvas) before timing

    time("1000 small filled circles", [] {
        for (int i = 0; i < 1000; ++i) canvas::filledCircle(0.5, 0.5, 0.01);
    });
    time("1000 large filled circles (r = 0.2)", [] {
        for (int i = 0; i < 1000; ++i) canvas::filledCircle(0.5, 0.5, 0.2);
    });
    time("1000 outline circles (r = 0.2)", [] {
        for (int i = 0; i < 1000; ++i) canvas::circle(0.5, 0.5, 0.2);
    });
    time("1000 corner-to-corner lines", [] {
        for (int i = 0; i < 1000; ++i) canvas::line(0, 0, 1, 1);
    });
    time("1000 text strings", [] {
        for (int i = 0; i < 1000; ++i) canvas::text(0.5, 0.5, "Hello, world 123");
    });
    time("100 clears", [] {
        for (int i = 0; i < 100; ++i) canvas::clear();
    });
    time("20000 points (immediate mode)", [] {
        for (int i = 0; i < 20000; ++i) canvas::point(i / 20000.0, 0.5);
    });

    const char* headless = std::getenv("CANVAS_HEADLESS");
    if (!(headless && *headless && *headless != '0')) {
        framePacing(false);
        framePacing(true);
    }
    std::fflush(stdout);
    std::_Exit(0);  // don't keep the window open
}
