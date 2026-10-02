// audio.hpp - sound as a sequence of samples, inspired by Princeton's StdAudio.
//
// Sound is a sequence of samples, numbers from -1 to +1 (values outside are
// clipped), at SAMPLE_RATE samples per second. A 440 Hz tone for one second:
//
//     #include <audio.hpp>
//     #include <cmath>
//
//     int main() {
//         const double pi = 3.14159265358979323846;
//         for (int i = 0; i < audio::SAMPLE_RATE; ++i) {
//             audio::play(0.5 * std::sin(2 * pi * 440 * i / audio::SAMPLE_RATE));
//         }
//     }
//
// play() keeps about a tenth of a second of sound queued and waits when the
// queue is full, so a loop that plays one sample at a time runs in real time.
// When the program ends, sound queued with play() finishes playing first;
// background sounds stop.
//
// If no audio device is available, a warning is printed once and the
// program continues without sound.
//
// Headless mode: if the environment variable CANVAS_HEADLESS is set to 1, no
// audio device is opened and play() returns immediately.
//
// Capture: if the environment variable CANVAS_AUDIO_CAPTURE is set to a file
// name ending in .wav, everything passed to play() is also written to that
// file when the program ends (background sounds are not included). This is
// useful for automated grading, with or without headless mode.
//
// Errors, such as a missing file, print a message and stop the program.

#ifndef CANVAS_AUDIO_HPP
#define CANVAS_AUDIO_HPP

#include <string>
#include <vector>

namespace audio {

// Samples per second.
constexpr int SAMPLE_RATE = 44100;

// ---------------------------------------------------------------------------
// Playing samples
// ---------------------------------------------------------------------------

// Plays one sample, or a sequence of samples. Waits when about a tenth of a
// second of sound is already queued.
void play(double sample);
void play(const std::vector<double>& samples);

// Waits until all sound queued with play() has finished playing.
void drain();

// ---------------------------------------------------------------------------
// Sound files
// ---------------------------------------------------------------------------

// Reads a .wav or .mp3 file as samples. Stereo is mixed down to mono and the
// sound is converted to SAMPLE_RATE.
std::vector<double> read(const std::string& filename);

// Writes samples to a .wav file (mono, 16-bit, SAMPLE_RATE).
void save(const std::string& filename, const std::vector<double>& samples);

// Plays a .wav or .mp3 file and waits until it has finished.
void play(const std::string& filename);

// ---------------------------------------------------------------------------
// Background sound, e.g. music and sound effects in a game
//
// These return immediately. Several sounds can play at once, together with
// play().
// ---------------------------------------------------------------------------

// Starts playing a .wav or .mp3 file, or samples, once.
void playInBackground(const std::string& filename);
void playInBackground(const std::vector<double>& samples);

// Starts playing a .wav or .mp3 file over and over.
void loopInBackground(const std::string& filename);

// Stops all background sounds.
void stopInBackground();

}  // namespace audio

#endif  // CANVAS_AUDIO_HPP
