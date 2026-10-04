// Tests for the color functions: gray(), hsv() and mix().

#include <canvas.hpp>
#include <image.hpp>

#include "check.hpp"

namespace {

bool near(canvas::Color a, canvas::Color b) {
    auto close = [](int x, int y) { return x - y <= 1 && y - x <= 1; };
    return close(a.r, b.r) && close(a.g, b.g) && close(a.b, b.b) && close(a.a, b.a);
}

static_assert(canvas::gray(128) == canvas::GRAY, "gray() is constexpr");

void testGray() {
    CHECK(canvas::gray(0) == canvas::BLACK);
    CHECK(canvas::gray(255) == canvas::WHITE);
    CHECK(canvas::gray(300) == canvas::WHITE);  // clamped
    CHECK(canvas::gray(-5) == canvas::BLACK);
}

void testHsv() {
    // The corners of the color wheel.
    CHECK(canvas::hsv(0, 1, 1) == canvas::RED);
    CHECK(canvas::hsv(60, 1, 1) == canvas::YELLOW);
    CHECK(canvas::hsv(120, 1, 1) == canvas::GREEN);
    CHECK(canvas::hsv(180, 1, 1) == canvas::CYAN);
    CHECK(canvas::hsv(240, 1, 1) == canvas::BLUE);
    CHECK(canvas::hsv(300, 1, 1) == canvas::MAGENTA);
    CHECK(near(canvas::hsv(30, 1, 1), canvas::rgb(255, 128, 0)));  // orange, halfway to yellow

    // Any angle works.
    CHECK(canvas::hsv(360, 1, 1) == canvas::RED);
    CHECK(canvas::hsv(420, 1, 1) == canvas::YELLOW);
    CHECK(canvas::hsv(-60, 1, 1) == canvas::MAGENTA);

    // Saturation 0 is gray, value 0 is black; outside 0 to 1 is clamped.
    CHECK(near(canvas::hsv(200, 0, 0.5), canvas::gray(128)));
    CHECK(canvas::hsv(200, 1, 0) == canvas::BLACK);
    CHECK(canvas::hsv(0, 2, 1.5) == canvas::RED);
    CHECK(near(canvas::hsv(0, 0.5, 1), canvas::rgb(255, 128, 128)));
}

void testMix() {
    CHECK(canvas::mix(canvas::BLACK, canvas::WHITE, 0) == canvas::BLACK);
    CHECK(canvas::mix(canvas::BLACK, canvas::WHITE, 1) == canvas::WHITE);
    CHECK(near(canvas::mix(canvas::BLACK, canvas::WHITE, 0.5), canvas::gray(128)));
    CHECK(near(canvas::mix(canvas::RED, canvas::BLUE, 0.25), canvas::rgb(191, 0, 64)));
    CHECK(canvas::mix(canvas::RED, canvas::BLUE, 7) == canvas::BLUE);  // clamped
    CHECK(near(canvas::mix(canvas::rgb(0, 0, 0, 0), canvas::RED, 0.5), canvas::rgb(128, 0, 0, 128)));  // alpha too
}

void testInImage() {
    // The same functions are available in namespace image.
    CHECK(image::hsv(120, 1, 1) == image::GREEN);
    CHECK(image::gray(0) == image::BLACK);
    CHECK(image::mix(image::WHITE, image::BLACK, 1) == image::BLACK);
}

}  // namespace

int main() {
    testGray();
    testHsv();
    testMix();
    testInImage();
    return finish();
}
