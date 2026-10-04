// recording.cpp - records the canvas as an animated GIF, for
// canvas::startRecording(). canvas.cpp hands over each frame with the time
// at which it appears; this file merges, times and encodes the frames.
//
// Frames are encoded as they arrive (with msf_gif), so a recording takes
// memory only for the compressed GIF, not for every frame.

#include <SDL3/SDL.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "internal.hpp"

#define MSF_GIF_IMPL
#include <msf_gif.h>

namespace canvas_internal {
namespace {

constexpr double kMaxSeconds = 60;  // a forgotten recording can't grow without end
constexpr int kMinDelay = 2;        // centiseconds; browsers slow shorter frames down to 10
constexpr int kLastFrameDelay = 100;  // the final picture stays for a second before the GIF repeats
constexpr int kQuality = 16;        // msf_gif's best

struct Recording {
    bool active = false;
    std::string filename;
    MsfGifState gif{};
    bool begun = false;  // msf_gif_begin() was called
    int width = 0, height = 0;
    image::Image pending;  // the latest frame, written once its length is known
    long pendingStart = 0;  // centiseconds
    int frames = 0;
};

Recording& rec() {
    static Recording* r = new Recording;  // never freed: saved by an exit handler (see internal.hpp)
    return *r;
}

void writePending(int delay) {
    Recording& r = rec();
    msf_gif_frame(&r.gif, reinterpret_cast<std::uint8_t*>(r.pending.pixels.data()), delay, kQuality, r.width * 4);
    ++r.frames;
}

bool samePixels(const image::Image& a, const image::Image& b) {
    return a.pixels.size() == b.pixels.size() &&
           std::memcmp(a.pixels.data(), b.pixels.data(), a.pixels.size() * sizeof(image::Color)) == 0;
}

// Writes the GIF and ends the recording at endTime (centiseconds): the last
// frame lasts until then, and then a second longer. Returns an error
// message, or "".
std::string finish(long endTime) {
    Recording& r = rec();
    r.active = false;
    writePending(static_cast<int>(endTime - r.pendingStart) + kLastFrameDelay);
    MsfGifResult result = msf_gif_end(&r.gif);
    r.begun = false;
    std::string error;
    if (!result.data) {
        error = "not enough memory to save '" + r.filename + "'";
    } else {
        SDL_IOStream* io = SDL_IOFromFile(r.filename.c_str(), "wb");  // handles UTF-8 names on Windows
        bool ok = io && SDL_WriteIO(io, result.data, result.dataSize) == result.dataSize;
        if (io && !SDL_CloseIO(io)) ok = false;
        if (!ok) error = "cannot write '" + r.filename + "'";
    }
    msf_gif_free(result);
    if (error.empty()) {
        const double seconds = static_cast<double>(endTime + kLastFrameDelay) / 100;
        std::fprintf(stderr, "canvas: saved the recording '%s' (%.1f seconds, %d frame%s)\n", r.filename.c_str(),
                     seconds, r.frames, r.frames == 1 ? "" : "s");
    }
    r.pending = image::Image();
    return error;
}

}  // namespace

bool recordingActive() { return rec().active; }

void recordingStart(const std::string& filename) {
    Recording& r = rec();
    if (lowerExtension(filename) != "gif") fail("canvas", "startRecording: '" + filename + "' must end in .gif");
    if (r.active) {
        fail("canvas", "startRecording: already recording to '" + r.filename + "' (call stopRecording() first)");
    }
    // Fail now, not when the recording ends, if the file can't be written.
    SDL_IOStream* io = SDL_IOFromFile(filename.c_str(), "wb");
    if (!io) fail("canvas", "startRecording: cannot write '" + filename + "'");
    SDL_CloseIO(io);
    r = Recording();
    r.active = true;
    r.filename = filename;
}

void recordingFrame(const image::Image& frame, double seconds) {
    Recording& r = rec();
    if (!r.active) return;
    const long time = std::lround(seconds * 100);
    if (!r.begun) {
        r.width = frame.width;
        r.height = frame.height;
        msf_gif_begin(&r.gif, r.width, r.height);
        r.begun = true;
        r.pending = frame;
        r.pendingStart = time;
        return;
    }
    if (frame.width != r.width || frame.height != r.height) return;  // the canvas size changed: skip
    if (samePixels(frame, r.pending)) return;  // nothing changed: the pending frame lasts longer
    if (time - r.pendingStart < kMinDelay) {
        r.pending = frame;  // too soon after the previous frame: show this one instead
    } else {
        writePending(static_cast<int>(time - r.pendingStart));
        r.pending = frame;
        r.pendingStart = time;
    }
    if (seconds >= kMaxSeconds) {
        std::fprintf(stderr, "canvas: the recording stopped after %.0f seconds, the longest it can be\n",
                     kMaxSeconds);
        std::string error = finish(time);
        if (error.empty()) return;
        if (!shuttingDown()) fail("canvas", "recording: " + error);
        std::fprintf(stderr, "canvas: recording: %s\n", error.c_str());
    }
}

void recordingFinish(const image::Image& last, double seconds, bool canFail) {
    if (!rec().active) return;
    recordingFrame(last, seconds);
    if (!rec().active) return;  // that frame reached the time limit, which saved it
    std::string error = finish(std::lround(seconds * 100));
    if (error.empty()) return;
    if (canFail) fail("canvas", "stopRecording: " + error);
    std::fprintf(stderr, "canvas: recording: %s\n", error.c_str());  // while exiting: report, don't exit
}

}  // namespace canvas_internal
