// Tests for startRecording() and stopRecording(). Runs headless; the GIFs are
// read back with stb_image.
//
// Usage: recording_test frames       (timing, merging, slow motion)
//        recording_test limit        (stops by itself after 60 seconds)
//        recording_test exit         (main returns without stopRecording())
//        recording_test check-exit   (the file from "exit" was saved)

#include <canvas.hpp>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_GIF
#include <stb_image.h>

#include "check.hpp"

namespace {

struct Gif {
    int width = 0, height = 0, frames = 0;
    std::vector<int> delays;  // milliseconds
    std::vector<unsigned char> rgba;

    // The colour in the middle of a frame.
    canvas::Color centre(int frame) const {
        const std::size_t i = (static_cast<std::size_t>(frame) * static_cast<std::size_t>(width * height) +
                               static_cast<std::size_t>(height / 2 * width + width / 2)) * 4;
        return canvas::Color{rgba[i], rgba[i + 1], rgba[i + 2], rgba[i + 3]};
    }

    int total() const {
        int ms = 0;
        for (int d : delays) ms += d;
        return ms;
    }
};

Gif read(const std::string& filename) {
    Gif gif;
    std::ifstream in(filename, std::ios::binary);
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    int* delays = nullptr;
    int channels = 0;
    unsigned char* data = stbi_load_gif_from_memory(bytes.data(), static_cast<int>(bytes.size()), &delays,
                                                    &gif.width, &gif.height, &gif.frames, &channels, 4);
    if (!data) {
        std::printf("cannot read %s\n", filename.c_str());
        return gif;
    }
    gif.delays.assign(delays, delays + gif.frames);
    gif.rgba.assign(data, data + static_cast<std::size_t>(gif.frames * gif.width * gif.height) * 4);
    stbi_image_free(data);
    stbi_image_free(delays);
    return gif;
}

// GIF colours are reduced to a palette, so allow a little difference.
bool near(canvas::Color a, canvas::Color b) {
    return std::abs(a.r - b.r) <= 8 && std::abs(a.g - b.g) <= 8 && std::abs(a.b - b.b) <= 8;
}

void testFrames() {
    canvas::setCanvasSize(40, 30);
    canvas::enableDoubleBuffering();
    canvas::setFrameRate(25);  // 40 ms per frame
    canvas::startRecording("rec_frames.gif");
    canvas::clear(canvas::RED);
    canvas::show();
    canvas::clear(canvas::BLUE);
    canvas::show();
    canvas::show();  // unchanged: the blue frame lasts twice as long
    canvas::clear(canvas::GREEN);
    canvas::pause(300);
    canvas::clear(canvas::BLACK);
    canvas::stopRecording();  // the final picture is shown for a second

    Gif gif = read("rec_frames.gif");
    CHECK(gif.width == 40 && gif.height == 30);
    CHECK(gif.frames == 4);
    if (gif.frames == 4) {
        CHECK((gif.delays == std::vector<int>{40, 80, 300, 1000}));
        CHECK(near(gif.centre(0), canvas::RED));
        CHECK(near(gif.centre(1), canvas::BLUE));
        CHECK(near(gif.centre(2), canvas::GREEN));
        CHECK(near(gif.centre(3), canvas::BLACK));
    }
    canvas::stopRecording();  // not recording: does nothing
}

void testWithoutFrameRate() {
    // Without a frame rate or pause(), a headless show() counts as 1/60 s.
    // GIF frames can't be shorter than 20 ms, so some are left out, but the
    // length stays right.
    canvas::setFrameRate(0);
    canvas::startRecording("rec_fast.gif");
    for (int i = 0; i < 60; ++i) {
        canvas::clear(i % 2 == 0 ? canvas::WHITE : canvas::BLACK);
        canvas::show();
    }
    canvas::stopRecording();
    Gif gif = read("rec_fast.gif");
    std::printf("60 frames without a frame rate: %d GIF frames, %d ms\n", gif.frames, gif.total());
    CHECK(gif.frames > 20 && gif.frames < 60);
    CHECK(std::abs(gif.total() - 2000) <= 20);  // 1 s of frames, then the last one for 1 s
    for (int d : gif.delays) CHECK(d >= 20);
}

void testSlowMotion() {
    // With setDrawDelay, each drawing call is a frame.
    canvas::disableDoubleBuffering();
    canvas::clear();
    canvas::setDrawDelay(100);
    canvas::startRecording("rec_slow.gif");
    canvas::setPenColor(canvas::RED);
    canvas::filledSquare(0.5, 0.5, 0.4);
    canvas::setPenColor(canvas::BLUE);
    canvas::filledSquare(0.5, 0.5, 0.2);
    canvas::stopRecording();
    canvas::setDrawDelay(0);
    Gif gif = read("rec_slow.gif");
    CHECK(gif.frames == 2);
    if (gif.frames == 2) {
        CHECK((gif.delays == std::vector<int>{100, 1100}));  // the last frame, then a second more
        CHECK(near(gif.centre(0), canvas::RED));
        CHECK(near(gif.centre(1), canvas::BLUE));
    }
}

void testLimit() {
    canvas::setCanvasSize(16, 16);
    canvas::enableDoubleBuffering();
    canvas::setFrameRate(50);
    canvas::startRecording("rec_limit.gif");
    for (int i = 0; i < 70 * 50; ++i) {  // 70 seconds of frames, each different
        canvas::clear(canvas::gray(i % 2 == 0 ? 0 : 255));
        canvas::show();
    }
    Gif gif = read("rec_limit.gif");  // already saved, when it reached 60 seconds
    CHECK(gif.frames > 0);
    CHECK(std::abs(gif.total() - 61000) <= 100);
    canvas::stopRecording();  // already stopped: does nothing
    canvas::startRecording("rec_limit2.gif");  // and a new recording can start
    canvas::stopRecording();
}

}  // namespace

int main(int argc, char** argv) {
    const std::string mode = argc > 1 ? argv[1] : "frames";
    if (mode == "frames") {
        testFrames();
        testWithoutFrameRate();
        testSlowMotion();
    } else if (mode == "limit") {
        testLimit();
    } else if (mode == "exit") {
        std::remove("rec_exit.gif");
        canvas::startRecording("rec_exit.gif");
        canvas::filledCircle(0.5, 0.5, 0.25);
        return 0;  // saved when the program ends
    } else if (mode == "check-exit") {
        Gif gif = read("rec_exit.gif");
        CHECK(gif.frames == 1);
        if (gif.frames == 1) CHECK(near(gif.centre(0), canvas::BLACK));
    } else {
        std::printf("unknown mode %s\n", mode.c_str());
        return 2;
    }
    return finish();
}
