// stopwatch.hpp - measuring how long code takes, inspired by Princeton's
// Stopwatch.
//
//     #include <stopwatch.hpp>
//
//     stopwatch::start();
//     sort(numbers);
//     std::cout << "sorting took " << stopwatch::elapsed() << " seconds\n";
//
// There is one stopwatch, like there is one canvas. It measures real
// (wall-clock) time, with a steady clock that doesn't jump when the
// computer's clock is changed.

#ifndef CANVAS_STOPWATCH_HPP
#define CANVAS_STOPWATCH_HPP

namespace stopwatch {

// Starts timing from now. Calling it again starts again from 0.
void start();

// The seconds since start() was last called, or since the program started if
// it wasn't. Reading it doesn't stop the stopwatch.
double elapsed();

}  // namespace stopwatch

#endif  // CANVAS_STOPWATCH_HPP
