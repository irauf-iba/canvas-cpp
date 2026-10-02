// Input and frame tests. SDL events are injected with SDL_PushEvent, so this
// runs without a person at the keyboard; with SDL_VIDEO_DRIVER=offscreen it
// also runs without a display.
//
// isKeyPressed() is not covered: SDL's keyboard state ignores injected events.

#include <draw.hpp>

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

void press(SDL_Keycode key) {
    SDL_Event e{};
    e.type = SDL_EVENT_KEY_DOWN;
    e.key.windowID = SDL_GetWindowID(window());
    e.key.key = key;
    e.key.down = true;
    SDL_PushEvent(&e);
}

std::string readTyped() {
    std::string s;
    while (draw::hasNextKeyTyped()) s += draw::nextKeyTyped();
    return s;
}

// Starts a new frame with no pending input.
void newFrame() {
    draw::show();
    draw::pause(0);
}

double elapsedMs(Uint64 since) { return static_cast<double>(SDL_GetTicksNS() - since) / 1e6; }

void testClick() {
    newFrame();
    click(0.75f, 0.25f);
    CHECK(draw::mouseClicked());
    CHECK(std::abs(draw::mouseX() - 0.5) < 0.01);  // scale is -1..1
    CHECK(std::abs(draw::mouseY() - 0.5) < 0.01);
    CHECK(!draw::mouseClicked());  // reported once
}

void testUnreadClickIsDropped() {
    newFrame();
    click(0.5f, 0.5f);
    CHECK(!draw::hasNextKeyTyped());  // reads the click event without consuming it
    draw::show();
    CHECK(!draw::mouseClicked());
}

void testClickDuringShowSurvivesPause() {
    newFrame();
    click(0.5f, 0.5f);
    draw::show();  // the click is read here, for the next frame
    draw::pause(0);
    CHECK(draw::mouseClicked());
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
    CHECK(draw::hasNextKeyTyped());
    draw::show();
    CHECK(!draw::hasNextKeyTyped());
}

void testPausePacing() {
    // 10 frames of 5 ms work plus pause(20) should take 10 x 20 ms, not 10 x 25.
    newFrame();
    Uint64 start = SDL_GetTicksNS();
    for (int i = 0; i < 10; ++i) {
        SDL_DelayPrecise(5'000'000);
        draw::pause(20);
    }
    double ms = elapsedMs(start);
    std::printf("10 x (5 ms work + pause(20)) took %.1f ms\n", ms);
    CHECK(ms > 190 && ms < 240);
}

void testPauseAfterGap() {
    // A one-off pause after a long gap waits the full time.
    draw::pause(0);
    SDL_DelayPrecise(300'000'000);
    Uint64 start = SDL_GetTicksNS();
    draw::pause(100);
    double ms = elapsedMs(start);
    std::printf("pause(100) after a gap took %.1f ms\n", ms);
    CHECK(ms > 98 && ms < 140);
}

}  // namespace

int main() {
    draw::setScale(-1, 1);
    draw::point(0, 0);  // opens the window

    testClick();
    testUnreadClickIsDropped();
    testClickDuringShowSurvivesPause();
    testTypedKeys();
    testKeyLimit();
    testReadKeysFreeSlots();
    testUnreadKeysAreDropped();
    testPausePacing();
    testPauseAfterGap();

    // Close the window so the program can end instead of waiting for the user.
    SDL_Event quit{};
    quit.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&quit);
    return finish();
}
