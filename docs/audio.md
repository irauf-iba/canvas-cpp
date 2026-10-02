# audio: sound as samples

`#include <audio.hpp>`

The `audio` module is a C++ version of Princeton's
[StdAudio](https://introcs.cs.princeton.edu/java/stdlib/javadoc/StdAudio.html).
Sound is a sequence of samples: numbers from −1 to +1, at
`audio::SAMPLE_RATE` (44,100) samples per second. Values outside −1 to +1 are
clipped.

```cpp
#include <audio.hpp>
#include <cmath>

int main() {
    const double PI = 3.14159265358979323846;
    for (int i = 0; i < audio::SAMPLE_RATE; ++i) {       // one second of A (440 Hz)
        audio::play(0.5 * std::sin(2 * PI * 440 * i / audio::SAMPLE_RATE));
    }
}
```

The header [`include/audio.hpp`](../include/audio.hpp) documents every function.

## Functions

### Playing samples

| Function | Does |
|---|---|
| `audio::play(sample)` | Plays one sample. |
| `audio::play(samples)` | Plays a `std::vector<double>` of samples. |
| `audio::drain()` | Waits until everything queued with `play()` has played. |

`play()` keeps about a tenth of a second of sound queued and waits when the
queue is full. A loop that plays one sample at a time therefore runs in real
time, as in the example above.

### Sound files

| Function | Does |
|---|---|
| `audio::read(file)` | Reads a .wav or .mp3 file as samples. Stereo is mixed down to mono, and the sound is converted to 44,100 samples per second. |
| `audio::save(file, samples)` | Writes a .wav file (mono, 16-bit, 44,100 Hz). |
| `audio::play(file)` | Plays a .wav or .mp3 file and waits until it has finished. |

### Background sound

| Function | Does |
|---|---|
| `audio::playInBackground(file)`, `audio::playInBackground(samples)` | Starts playing a sound once. |
| `audio::loopInBackground(file)` | Starts playing a sound over and over. |
| `audio::stopInBackground()` | Stops all background sounds. |

These return immediately. Several background sounds can play at once, mixed
together and with `play()`. Use them for music and sound effects in games.
Files are read once and remembered, so playing the same sound effect often is
cheap.

## Behavior

- **Program end.** Sound queued with `play()` finishes playing before the
  program ends, so the last note isn't cut off. Background sounds stop.
- **With draw.** While `play()` waits, the `draw` window stays responsive.
  `draw`'s input queries are cheap, so a Guitar Hero-style program can check
  the keyboard once per sample:

  ```cpp
  while (true) {
      if (draw::hasNextKeyTyped()) pluck(draw::nextKeyTyped());
      audio::play(nextSample());
  }
  ```

- **No sound device.** If there is none, for example over SSH or on a server,
  a warning is printed once and the program runs without sound.

## Headless mode and capture

If the environment variable `DRAW_HEADLESS` is set to 1, no audio device is
opened and `play()` returns immediately. A program that plays a minute of
music finishes in moments.

If `DRAW_AUDIO_CAPTURE` is set to a file name ending in .wav, everything
passed to `play()` is also written to that file when the program ends.
Background sounds are not included, since their timing depends on the machine.
Together these let a grader check a student's sound without a speaker:

```sh
DRAW_HEADLESS=1 DRAW_AUDIO_CAPTURE=out.wav ./student_program
```

## Errors

Mistakes stop the program with a message and exit status 1, for example:

```
audio: read: cannot open 'tune.wav'
audio: read: 'music.ogg' must end in .wav or .mp3
audio: read: 'tune.wav' is not a valid WAV file
audio: save: 'out.mp3' must end in .wav
```

## Differences from Princeton's StdAudio

| StdAudio | audio | Why |
|---|---|---|
| Reads WAV, AU, AIFF and MIDI | Reads WAV and MP3 | WAV and MP3 are the files students have. |
| `startRecording()`, `stopRecording()` | Not supported | Recording from a microphone is left for later. |
| `double[]` | `std::vector<double>` | The C++ equivalent. |
| Exceptions | Message and exit | Clearer for beginners than an uncaught exception. |

## How it works

- **Playback.** `play()` feeds an SDL audio stream from the caller's thread.
  Each background sound has its own stream, and SDL mixes them and converts to
  the device's format.
- **Looping.** Looping sounds are refilled from SDL's audio thread.
- **Files.** Sound files are read with dr_wav and dr_mp3, compiled into the
  library privately.
- **Sample rates.** Files at other sample rates are converted by linear
  interpolation, which is fine for course work but not for high-fidelity
  audio.

## Examples

- [`examples/scale.cpp`](../examples/scale.cpp): plays a C major scale one
  sample at a time, then saves it to `scale.wav`.
- [`examples/piano.cpp`](../examples/piano.cpp): a keyboard piano. Type
  `a s d f g h j k` or click the keys. It combines `draw` input with
  background notes.
