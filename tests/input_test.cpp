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
#include "internal.hpp"  // screenImage(), for the grid key

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

void move(float fx, float fy) {
    int w, h;
    SDL_GetWindowSize(window(), &w, &h);
    SDL_Event e{};
    e.type = SDL_EVENT_MOUSE_MOTION;
    e.motion.windowID = SDL_GetWindowID(window());
    e.motion.x = fx * static_cast<float>(w);
    e.motion.y = fy * static_cast<float>(h);
    SDL_PushEvent(&e);
}

// For SDL_AddTimer: presses the key, from the timer's thread, while the
// program waits in show() or pause().
SDL_WindowID gWindowID = 0;
Uint32 SDLCALL pressLater(void* key, SDL_TimerID, Uint32) {
    SDL_Event e{};
    e.type = SDL_EVENT_KEY_DOWN;
    e.key.windowID = gWindowID;
    e.key.key = *static_cast<const SDL_Keycode*>(key);
    e.key.down = true;
    SDL_PushEvent(&e);
    return 0;
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

void testFlippedYAxis() {
    // With y pointing down, the mouse's y grows downwards too.
    canvas::setYscale(1, -1);
    newFrame();
    click(0.5f, 0.25f);  // a quarter of the way down
    CHECK(canvas::mouseClicked());
    CHECK(std::abs(canvas::mouseY() - (-0.5)) < 0.01);
    canvas::setScale(-1, 1);  // back to the scale the other tests use
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
    CHECK(late < 12);                  // well under the 20 ms interval: show() didn't wait
    CHECK(next > 17 && next < 30);

    // 0 turns it off: 5 frames take much less than 5 intervals of 20 ms.
    canvas::setFrameRate(0);
    start = SDL_GetTicksNS();
    for (int i = 0; i < 5; ++i) canvas::show();
    CHECK(elapsedMs(start) < 50);
}

void testMouseCoordinatesInTitle() {
    canvas::setTitle("Test");
    canvas::showMouseCoordinates();
    SDL_Delay(40);  // the title is updated at most about 30 times a second
    SDL_Event e{};
    e.type = SDL_EVENT_MOUSE_MOTION;
    e.motion.windowID = SDL_GetWindowID(window());
    int w, h;
    SDL_GetWindowSize(window(), &w, &h);
    e.motion.x = 0.75f * static_cast<float>(w);
    e.motion.y = 0.25f * static_cast<float>(h);
    SDL_PushEvent(&e);
    canvas::pause(0);
    std::string title = SDL_GetWindowTitle(window());
    std::printf("title: %s\n", title.c_str());
    CHECK(title.rfind("Test | x 0.500, y 0.500", 0) == 0);  // scale is -1..1 here
    canvas::showMouseCoordinates(false);
    CHECK(std::string(SDL_GetWindowTitle(window())) == "Test");
}

void testDrawDelay() {
    canvas::setDrawDelay(20);
    Uint64 start = SDL_GetTicksNS();
    for (int i = 0; i < 5; ++i) canvas::point(0, 0);
    double ms = elapsedMs(start);
    canvas::setDrawDelay(0);
    std::printf("5 drawing calls with setDrawDelay(20) took %.1f ms\n", ms);
    CHECK(ms > 95 && ms < 200);
    start = SDL_GetTicksNS();
    for (int i = 0; i < 5; ++i) canvas::point(0, 0);
    CHECK(elapsedMs(start) < 50);  // off again
}

void testIsMouseOver() {
    newFrame();
    move(0.75f, 0.25f);  // (0.5, 0.5) with the scale -1..1
    CHECK(canvas::isMouseOver(0.5, 0.5, 0.1, 0.1));
    CHECK(canvas::isMouseOver(0.4, 0.45, 0.1, 0.05));  // on the edges counts
    CHECK(!canvas::isMouseOver(0, 0, 0.1, 0.1));
    CHECK(!canvas::isMouseOver(0.5, 0.7, 0.3, 0.1));
}

bool samePixels(const image::Image& a, const image::Image& b) { return a.pixels == b.pixels; }

void testDebugKeys() {
    static const SDL_Keycode keyN = SDLK_N, keyP = SDLK_P;
    gWindowID = SDL_GetWindowID(window());
    canvas::setTitle("Test");
    canvas::enableDebugKeys();
    newFrame();

    // P, and the 'p' it types, go to the debug keys, not to the program.
    press(SDLK_P);
    type("p");
    CHECK(!canvas::wasKeyPressed(canvas::Key::P));
    CHECK(!canvas::hasNextKeyTyped());
    CHECK(std::string(SDL_GetWindowTitle(window())).find("paused") != std::string::npos);

    // Paused at the next frame, until N runs one more frame ...
    SDL_AddTimer(200, pressLater, const_cast<SDL_Keycode*>(&keyN));
    Uint64 start = SDL_GetTicksNS();
    canvas::show();
    double ms = elapsedMs(start);
    std::printf("paused show() waited %.0f ms for N\n", ms);
    CHECK(ms > 180);

    // ... after which it pauses again, until P resumes.
    SDL_AddTimer(200, pressLater, const_cast<SDL_Keycode*>(&keyP));
    start = SDL_GetTicksNS();
    canvas::pause(0);
    ms = elapsedMs(start);
    std::printf("after one step, pause(0) waited %.0f ms for P\n", ms);
    CHECK(ms > 180);
    CHECK(std::string(SDL_GetWindowTitle(window())) == "Test");
    start = SDL_GetTicksNS();
    canvas::show();
    CHECK(elapsedMs(start) < 100);  // running again

    // G shows and hides the grid, over the canvas but not in it.
    newFrame();
    press(SDLK_G);
    CHECK(!canvas::wasKeyPressed(canvas::Key::G));
    CHECK(!samePixels(canvas_internal::screenImage(), canvas::snapshot()));
    press(SDLK_G);
    CHECK(!canvas::wasKeyPressed(canvas::Key::G));
    CHECK(samePixels(canvas_internal::screenImage(), canvas::snapshot()));

    // Turned off, P goes to the program again.
    canvas::disableDebugKeys();
    newFrame();
    press(SDLK_P);
    CHECK(canvas::wasKeyPressed(canvas::Key::P));
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
    testFlippedYAxis();
    testUnreadClickIsDropped();
    testClickDuringShowSurvivesPause();
    testTypedKeys();
    testKeyLimit();
    testReadKeysFreeSlots();
    testUnreadKeysAreDropped();
    testWasKeyPressed();
    testPausePacing();
    testFrameRate();
    testMouseCoordinatesInTitle();
    testDrawDelay();
    testIsMouseOver();
    testDebugKeys();
    testPauseAfterGap();

    // Close the window so the program can end instead of waiting for the user.
    SDL_Event quit{};
    quit.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&quit);
    return finish();
}
