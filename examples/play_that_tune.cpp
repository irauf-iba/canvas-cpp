// Reads notes from standard input and plays them. Each note is a pitch, in
// semitones above (or, if negative, below) A4 at 440 Hz, and a duration in
// seconds.
//
//     ./play_that_tune < examples/data/ode_to_joy.txt
//
// ode_to_joy.txt is the main theme of Beethoven's Ninth Symphony.
//
// After PlayThatTune in Sedgewick & Wayne, Computer Science: An
// Interdisciplinary Approach, Section 1.5. Written for this library.

#include <audio.hpp>

#include <cmath>
#include <iostream>

int main() {
    const double PI = 3.14159265358979323846;
    double pitch, duration;
    while (std::cin >> pitch >> duration) {
        double hz = 440 * std::pow(2, pitch / 12);
        int n = static_cast<int>(audio::SAMPLE_RATE * duration);
        for (int i = 0; i < n; ++i) {
            double fade = std::fmin(1.0, (n - i) / 400.0);  // no click between notes
            audio::play(0.5 * fade * std::sin(2 * PI * hz * i / audio::SAMPLE_RATE));
        }
    }
}
