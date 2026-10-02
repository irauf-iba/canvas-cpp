// Window lifetime tests. Each case runs in its own process.
//
// Usage: window_test keep-open | close-in-pause | close-while-drawing
//
//   keep-open            After main() returns, the window stays open until it
//                        is closed. A timer closes it after 300 ms.
//   close-in-pause       Closing the window during pause() ends the program
//                        with exit code 0.
//   close-while-drawing  Closing the window is noticed during a long run of
//                        drawing calls, without show() or pause().
//   image-error          An error in the image module closes an open window
//                        instead of keeping it open.

#include <canvas.hpp>
#include <image.hpp>

#include <SDL3/SDL.h>

#include <cstdio>
#include <cstdlib>
#include <string>

namespace {

Uint64 mainReturned = 0;

void pushQuit() {
    SDL_Event quit{};
    quit.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&quit);
}

Uint32 SDLCALL quitTimer(void*, SDL_TimerID, Uint32) {
    pushQuit();
    return 0;
}

// Registered before the library's own exit handler, so it runs after it.
void reportKeepOpen() {
    double ms = static_cast<double>(SDL_GetTicksNS() - mainReturned) / 1e6;
    std::printf("window stayed open for %.0f ms after main returned\n", ms);
    std::printf(ms >= 250 ? "PASSED\n" : "FAILED: closed too early\n");
}

}  // namespace

int main(int argc, char** argv) {
    const std::string test = argc > 1 ? argv[1] : "";

    if (test == "keep-open") {
        std::atexit(reportKeepOpen);
        canvas::filledCircle(0.5, 0.5, 0.25);
        SDL_AddTimer(300, quitTimer, nullptr);
        mainReturned = SDL_GetTicksNS();
        return 0;
    }

    if (test == "close-in-pause") {
        canvas::filledCircle(0.5, 0.5, 0.25);
        pushQuit();
        canvas::pause(2000);
        std::printf("FAILED: still running after the window was closed\n");
        return 1;
    }

    if (test == "close-while-drawing") {
        canvas::enableDoubleBuffering();
        canvas::point(0.5, 0.5);
        pushQuit();
        Uint64 start = SDL_GetTicksNS();
        while (SDL_GetTicksNS() - start < 3'000'000'000ULL) canvas::filledCircle(0.5, 0.5, 0.1);
        std::printf("FAILED: still running after the window was closed\n");
        return 1;
    }

    if (test == "image-error") {
        canvas::filledCircle(0.5, 0.5, 0.25);
        image::getPixel(canvas::snapshot(), 0, 512);  // one past the right edge
        std::printf("FAILED: no error reported\n");
        return 1;
    }

    std::printf("usage: window_test keep-open | close-in-pause | close-while-drawing | image-error\n");
    return 2;
}
