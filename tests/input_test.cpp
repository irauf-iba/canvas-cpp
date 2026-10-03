// Input and frame tests. SDL events are injected with SDL_PushEvent, so this
// runs without a person at the keyboard; with SDL_VIDEO_DRIVER=dummy it
// also runs without a display.
//
// isKeyPressed() is not covered: SDL's keyboard state ignores injected events.

#include <canvas.hpp>

#include <SDL3/SDL.h>

#include <cmath>
#include <string>

#include "check.hpp"

namespace {

SDL_Window* window() { return SDL_GetWindows(nullptr)[0]; }

void click(float fx, float fy) {
    int w, h;
    SDL_GetWindowSize(window(), &w, &h);
    SDL_Event e{};
    e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    e.button.windowID = SDL_GetWindowID(window());
    e.button.button = SDL_BUTTON_LEFT;
    e.button.down = true;
    e.button.x = fx * static_cast<float>(w);
    e.button.y = fy * static_cast<float>(h);
    SDL_PushEvent(&e);
}

void type(const char* text) {
    SDL_Event e{};
    e.type = SDL_EVENT_TEXT_INPUT;
    e.text.windowID = SDL_GetWindowID(window());
    e.text.text = text;
    SDL_PushEvent(&e);
}

void press(SDL_Keycode key, bool repeat = false) {
    SDL_Event e{};
    e.type = SDL_EVENT_KEY_DOWN;
    e.key.windowID = SDL_GetWindowID(window());
    e.key.key = key;
    e.key.down = true;
    e.key.repeat = repeat;
    SDL_PushEvent(&e);
}

std::string readTyped() {
    std::string s;
    while (canvas::hasNextKeyTyped()) s += canvas::nextKeyTyped();
    return s;
}

// Starts a new frame with no pending input.
void newFrame() {
    canvas::show();
    canvas::pause(0);
}

double elapsedMs(Uint64 since) { return static_cast<double>(SDL_GetTicksNS() - since) / 1e6; }

void testClick() {
    newFrame();
    click(0.75f, 0.25f);
    CHECK(canvas::mouseClicked());
    CHECK(std::abs(canvas::mouseX() - 0.5) < 0.01);  // scale is -1..1
    CHECK(std::abs(canvas::mouseY() - 0.5) < 0.01);
    CHECK(!canvas::mouseClicked());  // reported once
}

void testUnreadClickIsDropped() {
    newFrame();
    click(0.5f, 0.5f);
    CHECK(!canvas::hasNextKeyTyped());  // reads the click event without consuming it
    canvas::show();
    CHECK(!canvas::mouseClicked());
}

void testClickDuringShowSurvivesPause() {
    newFrame();
    click(0.5f, 0.5f);
    canvas::show();  // the click is read here, for the next frame
    canvas::pause(0);
    CHECK(canvas::mouseClicked());
}

void testTypedKeys() {
    newFrame();
    type("abc");
    press(SDLK_RETURN);
    press(SDLK_BACKSPACE);
    press(SDLK_TAB);
    press(SDLK_ESCAPE);
    type("\xC3\xA9");  // é: not ASCII, ignored
    CHECK(readTyped() == "abc\n\b\t\x1b");
}

void testKeyLimit() {
    newFrame();
    type("0123456789");
    type("ABCDEFGHIJ");
    CHECK(readTyped() == "0123456789ABCDEF");  // at most 16 unread keys
}

void testReadKeysFreeSlots() {
    // A program that polls keys without calling show() or pause() must keep
    // receiving them.
    newFrame();
    std::string all;
    for (int i = 0; i < 5; ++i) {
        type("abcdefgh");
        all += readTyped();
    }
    CHECK(all == "abcdefghabcdefghabcdefghabcdefghabcdefgh");
}

void testUnreadKeysAreDropped() {
    newFrame();
    type("x");
    CHECK(canvas::hasNextKeyTyped());
    canvas::show();
    CHECK(!canvas::hasNextKeyTyped());
}

void testWasKeyPressed() {
    newFrame();
    press(SDLK_LEFT);
    press(SDLK_A);
    CHECK(canvas::wasKeyPressed(canvas::Key::Left));
    CHECK(!canvas::wasKeyPressed(canvas::Key::Left));  // once per press
    CHECK(canvas::wasKeyPressed(canvas::Key::A));
    CHECK(!canvas::wasKeyPressed(canvas::Key::Right));

    newFrame();
    press(SDLK_RSHIFT);
    press(SDLK_UP, true);  // auto-repeat from holding a key doesn't count
    CHECK(canvas::wasKeyPressed(canvas::Key::Shift));
    CHECK(!canvas::wasKeyPressed(canvas::Key::Up));

    newFrame();
    press(SDLK_DOWN);
    CHECK(!canvas::hasNextKeyTyped());  // reads the event without consuming the press
    canvas::show();
    CHECK(!canvas::wasKeyPressed(canvas::Key::Down));  // unread presses are dropped
}

void testPausePacing() {
    // 10 frames of 5 ms work plus pause(20) should take 10 x 20 ms, not 10 x 25.
    newFrame();
    Uint64 start = SDL_GetTicksNS();
    for (int i = 0; i < 10; ++i) {
        SDL_DelayPrecise(5'000'000);
        canvas::pause(20);
    }
    double ms = elapsedMs(start);
    std::printf("10 x (5 ms work + pause(20)) took %.1f ms\n", ms);
    CHECK(ms > 190 && ms < 240);
}

void testFrameRate() {
    // 10 frames at 50 per second with 5 ms of work each take 10 x 20 ms.
    canvas::setFrameRate(50);
    canvas::show();  // starts the schedule
    Uint64 start = SDL_GetTicksNS();
    for (int i = 0; i < 10; ++i) {
        SDL_DelayPrecise(5'000'000);
        canvas::show();
    }
    double ms = elapsedMs(start);
    std::printf("10 frames at setFrameRate(50) with 5 ms work took %.1f ms\n", ms);
    CHECK(ms > 190 && ms < 240);

    // A late frame is shown at once; the next one waits a full interval.
    SDL_DelayPrecise(35'000'000);
    start = SDL_GetTicksNS();
    canvas::show();
    double late = elapsedMs(start);
    start = SDL_GetTicksNS();
    canvas::show();
    double next = elapsedMs(start);
    std::printf("late frame waited %.1f ms, the next %.1f ms\n", late, next);
    CHECK(late < 5);
    CHECK(next > 17 && next < 30);

    // 0 turns it off.
    canvas::setFrameRate(0);
    start = SDL_GetTicksNS();
    for (int i = 0; i < 5; ++i) canvas::show();
    CHECK(elapsedMs(start) < 10);
}

void testPauseAfterGap() {
    // A one-off pause after a long gap waits the full time.
    canvas::pause(0);
    SDL_DelayPrecise(300'000'000);
    Uint64 start = SDL_GetTicksNS();
    canvas::pause(100);
    double ms = elapsedMs(start);
    std::printf("pause(100) after a gap took %.1f ms\n", ms);
    CHECK(ms > 98 && ms < 140);
}

}  // namespace

int main() {
    canvas::setScale(-1, 1);
    canvas::point(0, 0);  // opens the window

    testClick();
    testUnreadClickIsDropped();
    testClickDuringShowSurvivesPause();
    testTypedKeys();
    testKeyLimit();
    testReadKeysFreeSlots();
    testUnreadKeysAreDropped();
    testWasKeyPressed();
    testPausePacing();
    testFrameRate();
    testPauseAfterGap();

    // Close the window so the program can end instead of waiting for the user.
    SDL_Event quit{};
    quit.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&quit);
    return finish();
}
