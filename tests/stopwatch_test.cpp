// Tests for stopwatch.hpp.

#include <stopwatch.hpp>

#include <chrono>
#include <thread>

#include "check.hpp"

int main() {
    // Without start(), it measures from the start of the program.
    CHECK(stopwatch::elapsed() >= 0);
    CHECK(stopwatch::elapsed() < 10);

    stopwatch::start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    const double t = stopwatch::elapsed();
    CHECK(t >= 0.05);
    CHECK(t < 5);                        // generous, for slow CI machines
    CHECK(stopwatch::elapsed() >= t);    // reading it doesn't stop it

    stopwatch::start();                  // starting again resets it
    CHECK(stopwatch::elapsed() < t);
    return finish();
}
