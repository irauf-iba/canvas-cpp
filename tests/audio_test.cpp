// Tests for audio.hpp that need no audio device: reading and writing files,
// mixing down, resampling, clipping and CANVAS_AUDIO_CAPTURE. Run headless,
// with CANVAS_AUDIO_CAPTURE=audio_test_capture.wav.
//
// Usage: audio_test <data-dir>

#include <audio.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "check.hpp"

namespace {

const double kPi = 3.14159265358979323846;
std::string dataDir;
std::vector<double> playedForCapture;

std::vector<double> tone(double hz, double seconds, double amplitude) {
    std::vector<double> s(static_cast<std::size_t>(seconds * audio::SAMPLE_RATE));
    for (std::size_t i = 0; i < s.size(); ++i) {
        s[i] = amplitude * std::sin(2 * kPi * hz * static_cast<double>(i) / audio::SAMPLE_RATE);
    }
    return s;
}

// Estimates the frequency from the number of upward zero crossings.
double frequency(const std::vector<double>& s) {
    int crossings = 0;
    for (std::size_t i = 1; i < s.size(); ++i) {
        if (s[i - 1] < 0 && s[i] >= 0) ++crossings;
    }
    return crossings * static_cast<double>(audio::SAMPLE_RATE) / static_cast<double>(s.size());
}

double peak(const std::vector<double>& s) {
    double p = 0;
    for (double x : s) p = std::max(p, std::abs(x));
    return p;
}

void testSaveAndRead() {
    std::vector<double> t = tone(440, 0.25, 0.8);
    audio::save("audio_test_tone.wav", t);
    std::vector<double> back = audio::read("audio_test_tone.wav");
    CHECK(back.size() == t.size());
    double maxError = 0;
    for (std::size_t i = 0; i < std::min(back.size(), t.size()); ++i) {
        maxError = std::max(maxError, std::abs(back[i] - t[i]));
    }
    CHECK(maxError < 1.0 / 16000);  // 16-bit quantization
}

void testClipping() {
    audio::save("audio_test_clip.wav", {-3.0, -1.0, 0.0, 1.0, 3.0, std::nan("")});
    std::vector<double> back = audio::read("audio_test_clip.wav");
    CHECK(back.size() == 6);
    CHECK(std::abs(back[0] + 1) < 1e-3 && std::abs(back[4] - 1) < 1e-3);
    CHECK(std::abs(back[5] + 1) < 1e-3);  // NaN becomes -1 rather than noise
}

void testStereoResampled() {
    // 0.5 s, 22050 Hz, stereo: a 440 Hz tone of amplitude 0.5 on the left,
    // silence on the right, so the mono mix has amplitude 0.25.
    std::vector<double> s = audio::read(dataDir + "/stereo22k.wav");
    std::printf("stereo22k.wav: %zu samples, %.1f Hz, peak %.3f\n", s.size(), frequency(s), peak(s));
    CHECK(std::abs(static_cast<double>(s.size()) - 0.5 * audio::SAMPLE_RATE) < 2);
    CHECK(std::abs(frequency(s) - 440) < 6);
    CHECK(std::abs(peak(s) - 0.25) < 0.01);
}

void testMp3() {
    // 0.5 s of 440 Hz at amplitude 0.5, mono MP3 (the decoder may add a
    // little padding).
    std::vector<double> s = audio::read(dataDir + "/tone440.mp3");
    std::printf("tone440.mp3: %zu samples, %.1f Hz, peak %.3f\n", s.size(), frequency(s), peak(s));
    CHECK(s.size() >= static_cast<std::size_t>(0.49 * audio::SAMPLE_RATE));
    CHECK(s.size() <= static_cast<std::size_t>(0.56 * audio::SAMPLE_RATE));
    CHECK(std::abs(frequency(s) - 440) < 10);
    CHECK(std::abs(peak(s) - 0.5) < 0.05);
}

void testMakingSounds() {
    // tone(): the length, frequency and volume, and a fade at both ends.
    std::vector<double> t = audio::tone(1000, 0.5, 0.8);
    CHECK(t.size() == static_cast<std::size_t>(0.5 * audio::SAMPLE_RATE));
    CHECK(std::abs(frequency(t) - 1000) < 5);
    CHECK(std::abs(peak(t) - 0.8) < 0.01);
    CHECK(t.front() == 0 && std::abs(t.back()) < 1e-3);
    std::vector<double> start(t.begin(), t.begin() + 44);  // the first millisecond
    CHECK(peak(start) < 0.8 * 0.25);
    CHECK(std::abs(peak(audio::tone(440, 0.1)) - 0.5) < 0.01);  // default volume 0.5

    // note(): semitones from A4.
    CHECK(std::abs(frequency(audio::note(0, 1)) - 440) < 2);
    CHECK(std::abs(frequency(audio::note(12, 1)) - 880) < 2);
    CHECK(std::abs(frequency(audio::note(-9, 1)) - 261.63) < 2);  // middle C
    CHECK(audio::note(3, 0.25, 0.1).size() == static_cast<std::size_t>(0.25 * audio::SAMPLE_RATE));

    // silence(), and sounds too short to fade fully.
    std::vector<double> rest = audio::silence(0.2);
    CHECK(rest.size() == static_cast<std::size_t>(0.2 * audio::SAMPLE_RATE));
    CHECK(peak(rest) == 0);
    CHECK(audio::tone(440, 0).empty());
    CHECK(audio::tone(440, 0.001).size() == 44);
}

void testHeadlessPlayReturnsImmediately() {
    // In headless mode play() doesn't wait for a device.
    std::vector<double> t = tone(220, 2.0, 0.3);
    auto start = std::chrono::steady_clock::now();
    audio::play(t);
    audio::play(0.25);
    audio::play(-0.25);
    audio::play(dataDir + "/stereo22k.wav");
    audio::drain();
    double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    std::printf("headless play of 2.5 s of sound took %.1f ms\n", ms);
    CHECK(ms < 500);
    playedForCapture = t;
    playedForCapture.push_back(0.25);
    playedForCapture.push_back(-0.25);
    std::vector<double> file = audio::read(dataDir + "/stereo22k.wav");
    playedForCapture.insert(playedForCapture.end(), file.begin(), file.end());
}

// Registered before the audio module's own exit handler, so it runs after
// the capture file has been written.
void checkCapture() {
    std::vector<double> captured = audio::read("audio_test_capture.wav");
    CHECK(captured.size() == playedForCapture.size());
    double maxError = 0;
    for (std::size_t i = 0; i < std::min(captured.size(), playedForCapture.size()); ++i) {
        maxError = std::max(maxError, std::abs(captured[i] - playedForCapture[i]));
    }
    std::printf("capture: %zu samples, max error %.6f\n", captured.size(), maxError);
    CHECK(maxError < 1.0 / 16000);
    std::fflush(stdout);
    std::_Exit(finish());
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::printf("usage: audio_test <data-dir>\n");
        return 2;
    }
    dataDir = argv[1];
    std::atexit(checkCapture);

    testSaveAndRead();
    testClipping();
    testStereoResampled();
    testMp3();
    testMakingSounds();
    testHeadlessPlayReturnsImmediately();
    return 0;  // checkCapture() reports the result
}
