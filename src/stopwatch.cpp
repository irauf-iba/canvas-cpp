// stopwatch.cpp - implementation of stopwatch.hpp.

#include "stopwatch.hpp"

#include <chrono>

namespace stopwatch {
namespace {

using Clock = std::chrono::steady_clock;

// Set when the program starts, so elapsed() works without start().
Clock::time_point startTime = Clock::now();

}  // namespace

void start() { startTime = Clock::now(); }

double elapsed() { return std::chrono::duration<double>(Clock::now() - startTime).count(); }

}  // namespace stopwatch
