// Tests that play through a real SDL audio device. CTest runs them with SDL's
// "dummy" driver, which consumes sound in real time without a speaker, or its
// "disk" driver, which writes the mixed output to a file.
//
// Usage: audio_device_test <case> [data-dir]
//
//   pacing          play() one sample at a time runs in real time, even when
//                   a draw input query is made for every sample (as in a
//                   Guitar Hero program); drain() waits for the end.
//   file            play(file) waits until the file has played.
//   background      Background sounds start, loop and stop without waiting.
//   exit-drain      Sound queued with play() finishes after main() returns.
//   disk-output     The mixed output of play() and a background sound
//                   reaches the device (needs SDL_AUDIO_DRIVER=disk).

#include <audio.hpp>
#include <draw.hpp>

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "check.hpp"

namespace {

const double kPi = 3.14159265358979323846;

double seconds(Uint64 since) { return static_cast<double>(SDL_GetTicksNS() - since) / 1e9; }

std::vector<double> tone(double hz, double length, double amplitude) {
    std::vector<double> s(static_cast<std::size_t>(length * audio::SAMPLE_RATE));
    for (std::size_t i = 0; i < s.size(); ++i) {
        s[i] = amplitude * std::sin(2 * kPi * hz * static_cast<double>(i) / audio::SAMPLE_RATE);
    }
    return s;
}

int pacing() {
    draw::point(0.5, 0.5);  // open a window, as a Guitar Hero program would
    Uint64 start = SDL_GetTicksNS();
    int keys = 0;
    for (int i = 0; i < audio::SAMPLE_RATE; ++i) {  // one second
        if (draw::hasNextKeyTyped()) keys += draw::nextKeyTyped();
        audio::play(0.3 * std::sin(2 * kPi * 440 * i / audio::SAMPLE_RATE));
    }
    double played = seconds(start);
    audio::drain();
    double drained = seconds(start);
    std::printf("1 s of samples: play loop %.3f s, after drain %.3f s\n", played, drained);
    CHECK(played > 0.8 && played < 1.15);   // real time, minus the ~0.1 s queue
    CHECK(drained > 0.95 && drained < 1.3);
    CHECK(keys == 0);
    SDL_Event quit{};
    quit.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&quit);  // so the window doesn't stay open
    return finish();
}

int file() {
    audio::save("audio_device_test.wav", tone(330, 0.4, 0.3));
    Uint64 start = SDL_GetTicksNS();
    audio::play("audio_device_test.wav");
    double t = seconds(start);
    std::printf("play(file) of 0.4 s took %.3f s\n", t);
    CHECK(t > 0.38 && t < 0.7);
    return finish();
}

int background() {
    audio::save("audio_device_test_loop.wav", tone(550, 0.1, 0.3));
    Uint64 start = SDL_GetTicksNS();
    audio::playInBackground(tone(440, 0.2, 0.3));
    audio::loopInBackground("audio_device_test_loop.wav");
    audio::playInBackground("audio_device_test_loop.wav");
    CHECK(seconds(start) < 0.05);  // returns immediately
    SDL_Delay(400);                // the loop keeps playing meanwhile
    audio::stopInBackground();
    audio::playInBackground(tone(660, 0.1, 0.3));  // works again after stopping
    SDL_Delay(150);
    audio::stopInBackground();
    return finish();
}

Uint64 mainReturned = 0;

void reportExitDrain() {
    double t = seconds(mainReturned);
    std::printf("exit waited %.3f s for queued sound\n", t);
    std::printf(t > 0.05 ? "PASSED\n" : "FAILED: queued sound was cut off\n");
}

int exitDrain() {
    std::atexit(reportExitDrain);  // runs after the audio module's handler
    audio::play(tone(440, 0.5, 0.3));
    mainReturned = SDL_GetTicksNS();
    return 0;
}

SDL_AudioSpec deviceSpec{};

// Reads the raw output the disk driver wrote, in the device's format. Runs
// after the audio module has closed the device, so the file is complete.
void checkDiskOutput() {
    std::ifstream in("audio_device_test_disk.raw", std::ios::binary);
    std::vector<char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::vector<float> samples;
    if (deviceSpec.format == SDL_AUDIO_F32) {
        samples.resize(bytes.size() / 4);
        std::memcpy(samples.data(), bytes.data(), samples.size() * 4);
    } else if (deviceSpec.format == SDL_AUDIO_S16) {
        std::vector<std::int16_t> pcm(bytes.size() / 2);
        std::memcpy(pcm.data(), bytes.data(), pcm.size() * 2);
        for (std::int16_t x : pcm) samples.push_back(static_cast<float>(x) / 32768.0f);
    }
    float peakValue = 0;
    for (float x : samples) peakValue = std::max(peakValue, std::abs(x));
    std::printf("disk output: %zu samples, peak %.3f\n", samples.size(), static_cast<double>(peakValue));
    CHECK(samples.size() > static_cast<std::size_t>(0.25 * deviceSpec.freq * deviceSpec.channels));
    CHECK(peakValue > 0.45 && peakValue <= 0.61);  // two tones of 0.3 mixed together
    std::fflush(stdout);
    std::_Exit(finish());
}

int diskOutput() {
    std::atexit(checkDiskOutput);  // runs after the audio module's handler
    audio::playInBackground(tone(660, 0.3, 0.3));
    audio::play(tone(440, 0.3, 0.3));
    audio::drain();

    int frames = 0;
    CHECK(SDL_GetAudioDeviceFormat(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &deviceSpec, &frames));
    std::printf("device format: %s, %d channel(s), %d Hz\n", SDL_GetAudioFormatName(deviceSpec.format),
                deviceSpec.channels, deviceSpec.freq);
    CHECK(deviceSpec.format == SDL_AUDIO_F32 || deviceSpec.format == SDL_AUDIO_S16);
    return 0;  // checkDiskOutput() reports the result
}

}  // namespace

int main(int argc, char** argv) {
    const std::string test = argc > 1 ? argv[1] : "";
    if (test == "pacing") return pacing();
    if (test == "file") return file();
    if (test == "background") return background();
    if (test == "exit-drain") return exitDrain();
    if (test == "disk-output") return diskOutput();
    std::printf("usage: audio_device_test pacing | file | background | exit-drain | disk-output\n");
    return 2;
}
