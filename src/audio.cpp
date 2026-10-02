// audio.cpp - implementation of audio.hpp.
//
// Sound goes to SDL audio streams bound to one playback device; SDL mixes
// them and converts to the device's format. play() feeds one stream from the
// caller's thread and waits when about a tenth of a second is queued; each
// background sound has its own stream. Looping sounds are refilled from
// SDL's audio thread through a stream callback.

#include "audio.hpp"

#include <SDL3/SDL.h>
#define SDL_MAIN_HANDLED
#define SDL_MAIN_NOIMPL
#include <SDL3/SDL_main.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "internal.hpp"

// dr_wav and dr_mp3, compiled into this file with internal linkage.
#define DRWAV_API static
#define DRWAV_PRIVATE static
#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>

#define DRMP3_API static
#define DRMP3_PRIVATE static
#define DR_MP3_IMPLEMENTATION
#include <dr_mp3.h>

namespace audio {
namespace {

// play() hands samples to SDL in blocks, and waits while more than
// kMaxQueued samples (about 93 ms) are waiting to be played.
constexpr std::size_t kBlock = 256;
constexpr int kMaxQueued = 4096;
constexpr int kBytesPerSample = static_cast<int>(sizeof(float));
constexpr SDL_AudioSpec kSpec = {SDL_AUDIO_F32, 1, SAMPLE_RATE};

using Sound = std::vector<float>;

// A sound playing in the background. A looping one is refilled by SDL's audio
// thread, which is the only thread that touches `position`.
struct Background {
    SDL_AudioStream* stream = nullptr;
    const Sound* loop = nullptr;
    std::size_t position = 0;
};

struct State {
    bool initialized = false;
    SDL_AudioDeviceID device = 0;  // 0: no sound (headless or no device)
    SDL_AudioStream* stream = nullptr;  // for play()
    std::vector<float> pending;         // samples not yet handed to SDL

    std::string capturePath;  // CANVAS_AUDIO_CAPTURE
    std::vector<std::int16_t> captured;

    std::vector<Background*> background;
    std::map<std::string, Sound> sounds;  // files, by name
};

// Allocated once and never freed, so it outlives the atexit handler.
State& st() {
    static State* s = new State;
    return *s;
}

[[noreturn]] void fail(const std::string& message) { canvas_internal::fail("audio", message); }

float clampSample(double x) {
    if (!(x >= -1)) return x > 1 ? 1.0f : -1.0f;  // also maps NaN to -1
    return x > 1 ? 1.0f : static_cast<float>(x);
}

std::int16_t toPcm16(float x) { return static_cast<std::int16_t>(std::lround(x * 32767.0f)); }

void onExit();

void openDevice() {
    State& s = st();
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDL_SetMainReady();
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        std::fprintf(stderr, "audio: no audio device (%s); continuing without sound\n", SDL_GetError());
        return;
    }
    s.device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (s.device == 0) {
        std::fprintf(stderr, "audio: cannot open the audio device (%s); continuing without sound\n",
                     SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return;
    }
    s.stream = SDL_CreateAudioStream(&kSpec, &kSpec);
    if (!s.stream || !SDL_BindAudioStream(s.device, s.stream)) {
        fail(std::string("cannot start playback: ") + SDL_GetError());
    }
}

void ensureInit() {
    State& s = st();
    if (s.initialized) return;
    s.initialized = true;
    if (const char* capture = std::getenv("CANVAS_AUDIO_CAPTURE"); capture && *capture) {
        s.capturePath = capture;
        if (canvas_internal::lowerExtension(s.capturePath) != "wav") {
            fail("CANVAS_AUDIO_CAPTURE must name a .wav file, not '" + s.capturePath + "'");
        }
    }
    if (!canvas_internal::headlessRequested()) openDevice();
    s.pending.reserve(kBlock);
    std::atexit(onExit);
}

// Waits while the play() stream holds more than `limit` samples.
void waitUntilQueuedAtMost(int limit) {
    State& s = st();
    while (SDL_GetAudioStreamQueued(s.stream) > limit * kBytesPerSample) {
        canvas_internal::keepWindowAlive();
        SDL_DelayNS(1'000'000);
    }
}

// Hands pending samples to SDL, then waits if too much is queued.
void submit() {
    State& s = st();
    if (s.pending.empty()) return;
    SDL_PutAudioStreamData(s.stream, s.pending.data(), static_cast<int>(s.pending.size()) * kBytesPerSample);
    s.pending.clear();
    waitUntilQueuedAtMost(kMaxQueued);
}

void enqueue(float sample) {
    State& s = st();
    if (!s.capturePath.empty()) s.captured.push_back(toPcm16(sample));
    if (s.device == 0) return;
    s.pending.push_back(sample);
    if (s.pending.size() >= kBlock) submit();
}

void drainNow() {
    State& s = st();
    if (s.device == 0) return;
    submit();
    SDL_FlushAudioStream(s.stream);
    waitUntilQueuedAtMost(0);
    // The device still holds one buffer that it is playing.
    SDL_AudioSpec spec;
    int frames = 0;
    if (SDL_GetAudioDeviceFormat(s.device, &spec, &frames) && spec.freq > 0) {
        Uint64 end = SDL_GetTicksNS() + static_cast<Uint64>(frames) * 1'000'000'000ULL / static_cast<Uint64>(spec.freq) + 5'000'000;
        while (SDL_GetTicksNS() < end) {
            canvas_internal::keepWindowAlive();
            SDL_DelayNS(1'000'000);
        }
    }
}

// --- files -------------------------------------------------------------------

// Linear interpolation; adequate for course work.
Sound resample(const std::vector<float>& in, unsigned rate) {
    if (rate == static_cast<unsigned>(SAMPLE_RATE) || in.empty()) return in;
    const double step = static_cast<double>(rate) / SAMPLE_RATE;
    const std::size_t n = static_cast<std::size_t>(std::llround(static_cast<double>(in.size()) / step));
    Sound out(n);
    for (std::size_t i = 0; i < n; ++i) {
        double pos = static_cast<double>(i) * step;
        std::size_t j = static_cast<std::size_t>(pos);
        double f = pos - static_cast<double>(j);
        float a = in[std::min(j, in.size() - 1)], b = in[std::min(j + 1, in.size() - 1)];
        out[i] = static_cast<float>(a + (b - a) * f);
    }
    return out;
}

Sound readFile(const std::string& filename, const char* function) {
    const std::string ext = canvas_internal::lowerExtension(filename);
    if (ext != "wav" && ext != "mp3") {
        fail(std::string(function) + ": '" + filename + "' must end in .wav or .mp3");
    }
    const std::string path = canvas_internal::findInputFile(filename);
    if (path.empty()) fail(std::string(function) + ": " + canvas_internal::notFound(filename));

    unsigned channels = 0, rate = 0;
    std::uint64_t frames = 0;
    float* data = nullptr;
    if (ext == "wav") {
        drwav_uint64 n = 0;
        data = drwav_open_file_and_read_pcm_frames_f32(path.c_str(), &channels, &rate, &n, nullptr);
        frames = n;
    } else {
        drmp3_config config{};
        drmp3_uint64 n = 0;
        data = drmp3_open_file_and_read_pcm_frames_f32(path.c_str(), &config, &n, nullptr);
        channels = config.channels;
        rate = config.sampleRate;
        frames = n;
    }
    auto release = [&] {
        if (ext == "wav") {
            drwav_free(data, nullptr);
        } else {
            drmp3_free(data, nullptr);
        }
    };
    if (!data || channels == 0 || rate == 0) {
        if (data) release();
        fail(std::string(function) + ": '" + filename + "' is not a valid " +
             (ext == "wav" ? "WAV" : "MP3") + " file");
    }

    std::vector<float> mono(static_cast<std::size_t>(frames));
    for (std::size_t i = 0; i < mono.size(); ++i) {
        float sum = 0;
        for (unsigned c = 0; c < channels; ++c) sum += data[i * channels + c];
        mono[i] = sum / static_cast<float>(channels);
    }
    release();
    return resample(mono, rate);
}

// Files are read once and kept, since games play the same sounds often.
const Sound& sound(const std::string& filename, const char* function) {
    State& s = st();
    auto it = s.sounds.find(filename);
    if (it == s.sounds.end()) it = s.sounds.emplace(filename, readFile(filename, function)).first;
    return it->second;
}

void writeWav(const std::string& filename, const std::vector<std::int16_t>& pcm, const char* function) {
    drwav_data_format format{};
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_PCM;
    format.channels = 1;
    format.sampleRate = SAMPLE_RATE;
    format.bitsPerSample = 16;
    drwav wav;
    if (!drwav_init_file_write(&wav, filename.c_str(), &format, nullptr)) {
        fail(std::string(function) + ": cannot write '" + filename + "'");
    }
    drwav_uint64 written = drwav_write_pcm_frames(&wav, pcm.size(), pcm.data());
    drwav_uninit(&wav);
    if (written != pcm.size()) fail(std::string(function) + ": cannot write '" + filename + "'");
}

// --- background ----------------------------------------------------------------

// Keeps a looping stream supplied with samples, wrapping around the sound.
void SDLCALL refillLoop(void* userdata, SDL_AudioStream* stream, int additional, int) {
    Background* b = static_cast<Background*>(userdata);
    const Sound& data = *b->loop;
    if (data.empty()) return;
    std::size_t needed = static_cast<std::size_t>(std::max(additional / kBytesPerSample, 1024));
    while (needed > 0) {
        std::size_t n = std::min(needed, data.size() - b->position);
        SDL_PutAudioStreamData(stream, data.data() + b->position, static_cast<int>(n) * kBytesPerSample);
        b->position = (b->position + n) % data.size();
        needed -= n;
    }
}

// Frees background sounds that have finished.
void removeFinished() {
    State& s = st();
    auto finished = [](Background* b) {
        if (b->loop || SDL_GetAudioStreamQueued(b->stream) > 0) return false;
        SDL_DestroyAudioStream(b->stream);
        delete b;
        return true;
    };
    s.background.erase(std::remove_if(s.background.begin(), s.background.end(), finished),
                       s.background.end());
}

void startBackground(const Sound& samples, const Sound* loop) {
    State& s = st();
    if (s.device == 0 || (samples.empty() && !loop)) return;
    removeFinished();
    Background* b = new Background;
    b->stream = SDL_CreateAudioStream(&kSpec, &kSpec);
    if (!b->stream) fail(std::string("cannot start a background sound: ") + SDL_GetError());
    if (loop) {
        b->loop = loop;
        SDL_SetAudioStreamGetCallback(b->stream, refillLoop, b);
    } else {
        SDL_PutAudioStreamData(b->stream, samples.data(), static_cast<int>(samples.size()) * kBytesPerSample);
        SDL_FlushAudioStream(b->stream);
    }
    if (!SDL_BindAudioStream(s.device, b->stream)) {
        fail(std::string("cannot start a background sound: ") + SDL_GetError());
    }
    s.background.push_back(b);
}

void stopAllBackground() {
    State& s = st();
    for (Background* b : s.background) {
        SDL_DestroyAudioStream(b->stream);  // waits for a running callback to finish
        delete b;
    }
    s.background.clear();
}

void onExit() {
    canvas_internal::beginShutdown();
    State& s = st();
    if (!canvas_internal::failing()) drainNow();  // let the last notes finish
    if (!s.capturePath.empty()) writeWav(s.capturePath, s.captured, "CANVAS_AUDIO_CAPTURE");
    if (s.device == 0) return;
    stopAllBackground();
    SDL_DestroyAudioStream(s.stream);
    SDL_CloseAudioDevice(s.device);
    s.device = 0;
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

}  // namespace

// ===========================================================================
// Public API
// ===========================================================================

void play(double sample) {
    ensureInit();
    enqueue(clampSample(sample));
}

void play(const std::vector<double>& samples) {
    ensureInit();
    for (double x : samples) enqueue(clampSample(x));
}

void drain() {
    ensureInit();
    drainNow();
}

std::vector<double> read(const std::string& filename) {
    const Sound& samples = sound(filename, "read");
    return std::vector<double>(samples.begin(), samples.end());
}

void save(const std::string& filename, const std::vector<double>& samples) {
    if (canvas_internal::lowerExtension(filename) != "wav") {
        fail("save: '" + filename + "' must end in .wav");
    }
    std::vector<std::int16_t> pcm;
    pcm.reserve(samples.size());
    for (double x : samples) pcm.push_back(toPcm16(clampSample(x)));
    writeWav(filename, pcm, "save");
}

void play(const std::string& filename) {
    ensureInit();
    for (float x : sound(filename, "play")) enqueue(x);
    drainNow();
}

void playInBackground(const std::string& filename) {
    ensureInit();
    startBackground(sound(filename, "playInBackground"), nullptr);
}

void playInBackground(const std::vector<double>& samples) {
    ensureInit();
    Sound converted;
    converted.reserve(samples.size());
    for (double x : samples) converted.push_back(clampSample(x));
    startBackground(converted, nullptr);
}

void loopInBackground(const std::string& filename) {
    ensureInit();
    const Sound& samples = sound(filename, "loopInBackground");
    if (samples.empty()) return;
    startBackground(samples, &samples);
}

void stopInBackground() {
    ensureInit();
    stopAllBackground();
}

}  // namespace audio
