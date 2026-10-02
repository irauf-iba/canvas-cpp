// Speaker test, driven by tests/manual/speaker_test.sh.
//
// Usage: speaker_test play
//        speaker_test check recording.wav
//
// "play" plays a known sequence through the default output device:
//   1. a C major scale, eight notes of 0.3 s, played with play();
//   2. 0.3 s of silence;
//   3. 0.6 s of a 440 Hz tone with play() while a 660 Hz tone plays in the
//      background, to check that the device output mixes them.
// "check" analyses a recording of that output: the scale's length, each
// note's pitch, how closely it matches the samples played, and whether
// both tones are present in the mixed section.

#include <audio.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {

const double kPi = 3.14159265358979323846;
const int kRate = audio::SAMPLE_RATE;
const int kSemitones[] = {0, 2, 4, 5, 7, 9, 11, 12};
const char* kNames[] = {"C4", "D4", "E4", "F4", "G4", "A4", "B4", "C5"};
const double kNoteSeconds = 0.3;
const double kGapSeconds = 0.3;
const double kMixSeconds = 0.6;

double noteHz(int semitones) { return 261.63 * std::pow(2.0, semitones / 12.0); }

std::vector<double> tone(double hz, double seconds, double amplitude) {
    int n = static_cast<int>(seconds * kRate);
    std::vector<double> s(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        double fade = std::min(1.0, (n - i) / 2000.0);  // no click at the end
        s[static_cast<std::size_t>(i)] = amplitude * fade * std::sin(2 * kPi * hz * i / kRate);
    }
    return s;
}

std::vector<double> scale() {
    std::vector<double> all;
    for (int semitones : kSemitones) {
        std::vector<double> n = tone(noteHz(semitones), kNoteSeconds, 0.4);
        all.insert(all.end(), n.begin(), n.end());
    }
    return all;
}

int play() {
    auto start = std::chrono::steady_clock::now();
    audio::play(scale());
    audio::play(std::vector<double>(static_cast<std::size_t>(kGapSeconds * kRate), 0.0));
    audio::playInBackground(tone(660, kMixSeconds, 0.25));
    audio::play(tone(440, kMixSeconds, 0.25));
    audio::drain();
    double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::printf("played the test sequence in %.3f s (expected about %.1f s)\n", s,
                8 * kNoteSeconds + kGapSeconds + kMixSeconds);
    return 0;
}

// Magnitude of frequency hz in x[from, from + n), by the Goertzel algorithm.
double magnitude(const std::vector<double>& x, std::size_t from, std::size_t n, double hz) {
    double coeff = 2 * std::cos(2 * kPi * hz / kRate), s1 = 0, s2 = 0;
    for (std::size_t i = from; i < from + n && i < x.size(); ++i) {
        double s0 = x[i] + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }
    return std::sqrt(std::max(0.0, s1 * s1 + s2 * s2 - coeff * s1 * s2));
}

// The strongest frequency within 20 Hz of `expected`, to 0.1 Hz.
double strongestNear(const std::vector<double>& x, std::size_t from, std::size_t n, double expected) {
    double best = expected, bestMagnitude = -1;
    for (double hz = expected - 20; hz <= expected + 20; hz += 0.1) {
        double m = magnitude(x, from, n, hz);
        if (m > bestMagnitude) {
            bestMagnitude = m;
            best = hz;
        }
    }
    return best;
}

int check(const std::string& filename) {
    const std::vector<double> rec = audio::read(filename);
    const std::vector<double> ref = scale();
    int failures = 0;
    auto result = [&](bool ok, const std::string& what) {
        std::printf("%s %s\n", ok ? "  ok  " : "  FAIL", what.c_str());
        if (!ok) ++failures;
    };

    // Where the sound starts.
    std::size_t onset = 0;
    while (onset < rec.size() && std::abs(rec[onset]) < 0.01) ++onset;
    if (onset + ref.size() > rec.size()) {
        std::printf("  FAIL no test sequence found in %s (%.1f s long)\n", filename.c_str(),
                    static_cast<double>(rec.size()) / kRate);
        return 1;
    }

    // Align the recording with the scale that was played.
    std::size_t lag = onset;
    double bestCorrelation = -1;
    std::size_t lo = onset > 2000 ? onset - 2000 : 0;
    for (std::size_t l = lo; l <= onset + 500 && l + ref.size() <= rec.size(); ++l) {
        double dot = 0, norm = 0;
        for (std::size_t i = 0; i < ref.size(); i += 4) {  // every 4th sample is plenty
            dot += rec[l + i] * ref[i];
            norm += rec[l + i] * rec[l + i];
        }
        double c = norm > 0 ? dot / std::sqrt(norm) : 0;
        if (c > bestCorrelation) {
            bestCorrelation = c;
            lag = l;
        }
    }
    double dot = 0, rr = 0, pp = 0, peak = 0;
    for (std::size_t i = 0; i < ref.size(); ++i) {
        dot += rec[lag + i] * ref[i];
        rr += rec[lag + i] * rec[lag + i];
        pp += ref[i] * ref[i];
        peak = std::max(peak, std::abs(rec[lag + i]));
    }
    double correlation = dot / std::sqrt(rr * pp);
    std::printf("scale found at %.3f s; recorded peak %.3f (played 0.400; differs if the system\n"
                "applies volume before the monitor)\n",
                static_cast<double>(lag) / kRate, peak);

    // The scale ends where the sound drops below the threshold.
    std::size_t end = lag + ref.size() + kRate / 10;
    while (end > lag && std::abs(rec[end - 1]) < 0.01) --end;
    double length = static_cast<double>(end - lag) / kRate;
    char line[200];
    std::snprintf(line, sizeof line, "scale lasts %.3f s (expected %.3f s)", length, 8 * kNoteSeconds);
    result(std::abs(length - 8 * kNoteSeconds) < 0.03, line);
    std::snprintf(line, sizeof line, "matches the samples played: correlation %.4f", correlation);
    result(correlation > 0.98, line);

    const std::size_t noteLength = static_cast<std::size_t>(kNoteSeconds * kRate);
    for (int i = 0; i < 8; ++i) {
        double expected = noteHz(kSemitones[i]);
        std::size_t from = lag + static_cast<std::size_t>(i) * noteLength + 1000;
        double measured = strongestNear(rec, from, noteLength - 3000, expected);
        std::snprintf(line, sizeof line, "%s: %.1f Hz (expected %.1f Hz)", kNames[i], measured, expected);
        result(std::abs(measured - expected) < 1.0, line);
    }

    // Mixed section: both tones present, compared with a frequency between them.
    std::size_t mix = lag + ref.size() + static_cast<std::size_t>(kGapSeconds * kRate) + kRate / 20;
    std::size_t mixLength = static_cast<std::size_t>((kMixSeconds - 0.15) * kRate);
    if (mix + mixLength > rec.size()) {
        result(false, "the recording ends before the mixed section (stopped too early?)");
        std::printf("speaker test FAILED\n");
        return 1;
    }
    double m440 = magnitude(rec, mix, mixLength, 440), m660 = magnitude(rec, mix, mixLength, 660);
    double m550 = magnitude(rec, mix, mixLength, 550);
    std::snprintf(line, sizeof line, "mixed section has 440 Hz from play() (%.0fx the level at 550 Hz)",
                  m440 / std::max(m550, 1e-9));
    result(m440 > 20 * m550, line);
    std::snprintf(line, sizeof line, "mixed section has 660 Hz from the background (%.0fx the level at 550 Hz)",
                  m660 / std::max(m550, 1e-9));
    result(m660 > 20 * m550, line);

    std::printf(failures == 0 ? "speaker test passed\n" : "speaker test FAILED\n");
    return failures == 0 ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string mode = argc > 1 ? argv[1] : "";
    if (mode == "play" && argc == 2) return play();
    if (mode == "check" && argc == 3) return check(argv[2]);
    std::printf("usage: speaker_test play\n       speaker_test check recording.wav\n");
    return 2;
}
