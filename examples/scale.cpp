// Plays a C major scale one sample at a time, then saves it to scale.wav.

#include <audio.hpp>

#include <cmath>
#include <vector>

const double PI = 3.14159265358979323846;

// A note `semitones` above middle C (C4 = 261.63 Hz), as samples.
std::vector<double> note(int semitones, double seconds) {
    double hz = 261.63 * std::pow(2.0, semitones / 12.0);
    int n = static_cast<int>(seconds * audio::SAMPLE_RATE);
    std::vector<double> samples(n);
    for (int i = 0; i < n; ++i) {
        double fade = std::min(1.0, (n - i) / 2000.0);  // avoid a click at the end
        samples[i] = 0.4 * fade * std::sin(2 * PI * hz * i / audio::SAMPLE_RATE);
    }
    return samples;
}

int main() {
    const int scale[] = {0, 2, 4, 5, 7, 9, 11, 12};
    std::vector<double> all;
    for (int semitones : scale) {
        for (double sample : note(semitones, 0.3)) {
            audio::play(sample);
            all.push_back(sample);
        }
    }
    audio::save("scale.wav", all);
}
