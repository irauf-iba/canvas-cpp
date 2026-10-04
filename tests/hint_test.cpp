// Hints: which mistakes give one, which look-alikes don't, and that each kind
// is printed only once. stderr goes to a file that is checked at the end.
//
// Usage: hint_test drawing     (headless)
//        hint_test no-show     (with a window: double buffering, no show())
//        hint_test off         (setHints(false))
//        hint_test env         (run with CANVAS_HINTS=0)
//        hint_test flipped     (a y axis pointing down, in pixel coordinates)

#include <canvas.hpp>
#include <image.hpp>

#include <SDL3/SDL.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "check.hpp"

namespace {

std::string hintsSoFar() {
    std::fflush(stderr);
    std::ifstream in("hint_test_stderr.txt");
    std::stringstream text;
    text << in.rdbuf();
    return text.str();
}

int count(const std::string& text, const std::string& part) {
    int n = 0;
    for (std::size_t at = text.find(part); at != std::string::npos; at = text.find(part, at + 1)) ++n;
    return n;
}

void drawing() {
    // Look-alikes that must not give a hint.
    canvas::filledCircle(0.5, 0.5, 0);          // zero size, but inside the canvas
    canvas::filledCircle(-0.1, 0.5, 0.3);       // partly outside
    canvas::setPenColor(canvas::RED);
    canvas::filledCircle(0.5, 0.5, 0.2);
    canvas::setPenColor(canvas::WHITE);
    canvas::filledCircle(0.45, 0.55, 0.03);     // white eyes on a red face are fine
    canvas::picture(0.2, 0.2, image::create(4, 4, image::RED));  // pictures don't use the pen
    CHECK(hintsSoFar().empty());

    canvas::setPenColor(canvas::BLACK);
    canvas::circle(200, 150, 50);               // pixel coordinates by mistake
    canvas::circle(300, 150, 50);               // the same mistake again: no second hint
    canvas::setPenColor(canvas::WHITE);
    canvas::filledCircle(0.8, 0.2, 0.05);       // white on white
    canvas::setPenColor(canvas::rgb(255, 0, 0, 0));
    canvas::line(0, 0, 1, 1);                   // a fully transparent pen

    std::string hints = hintsSoFar();
    std::printf("%s", hints.c_str());
    CHECK(count(hints, "canvas: hint: circle at (200, 150) is outside the visible area "
                       "(x from 0 to 1, y from 0 to 1).") == 1);
    CHECK(count(hints, "outside the visible area") == 1);
    CHECK(count(hints, "filledCircle at (0.8, 0.2) can't be seen: it is drawn in WHITE on a WHITE background") == 1);
    CHECK(count(hints, "the pen colour is fully transparent (alpha 0), so line draws nothing") == 1);
    CHECK(count(hints, "canvas: hint:") == 3);
}

void noShow() {
    canvas::enableDoubleBuffering();
    Uint64 start = SDL_GetTicksNS();
    while (SDL_GetTicksNS() - start < 2'300'000'000ULL) {  // over 2 s of drawing, never shown
        canvas::clear();
        canvas::filledCircle(0.5, 0.5, 0.1);
        SDL_Delay(10);
    }
    std::string hints = hintsSoFar();
    std::printf("%s", hints.c_str());
    CHECK(count(hints, "enableDoubleBuffering() is on, so drawing appears on screen only when show() is called") == 1);
    SDL_Event quit{};
    quit.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&quit);  // so the window doesn't stay open
}

void flipped() {
    // Pixel coordinates with the origin at the top left, as in many graphics
    // libraries: a 400 x 300 canvas, x from 0 to 400, y from 0 down to 300.
    canvas::setCanvasSize(400, 300);
    canvas::setXscale(0, 400);
    canvas::setYscale(300, 0);
    canvas::setPenColor(canvas::BOOK_RED);
    canvas::filledRectangle(50, 30, 40, 20);  // pixels 10..90 across, 10..50 down
    image::Image img = canvas::snapshot();
    CHECK(img[20][20] == canvas::BOOK_RED);       // near the top left
    CHECK(img[45][85] == canvas::BOOK_RED);
    CHECK(img[60][20] == canvas::WHITE);          // below it
    CHECK(img[280][20] == canvas::WHITE);         // not at the bottom left

    canvas::circle(500, 150, 20);  // off to the right
    std::string hints = hintsSoFar();
    std::printf("%s", hints.c_str());
    CHECK(count(hints, "canvas: hint: circle at (500, 150) is outside the visible area "
                       "(x from 0 to 400, y from 300 to 0). Check the coordinates against the scale.") == 1);
}

void off(bool callSetHints) {
    if (callSetHints) canvas::setHints(false);
    canvas::circle(200, 150, 50);
    canvas::setPenColor(canvas::WHITE);
    canvas::filledCircle(0.8, 0.2, 0.05);
    CHECK(hintsSoFar().empty());
}

}  // namespace

int main(int argc, char** argv) {
    const std::string mode = argc > 1 ? argv[1] : "";
    std::freopen("hint_test_stderr.txt", "w", stderr);
    if (mode == "drawing") drawing();
    else if (mode == "no-show") noShow();
    else if (mode == "off") off(true);
    else if (mode == "env") off(false);
    else if (mode == "flipped") flipped();
    else return 2;
    return finish();
}
