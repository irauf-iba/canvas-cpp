// Tests for the game helpers: distance and overlap checks. (isMouseOver is
// tested in input_test, which can move the mouse.)

#include <canvas.hpp>

#include <cmath>

#include "check.hpp"

namespace {

void testDistance() {
    CHECK(std::abs(canvas::distance(0, 0, 3, 4) - 5) < 1e-12);
    CHECK(std::abs(canvas::distance(1, 1, 1, 1)) < 1e-12);
    CHECK(std::abs(canvas::distance(-1, 2, 2, -2) - 5) < 1e-12);
}

void testCircles() {
    CHECK(canvas::circlesOverlap(0.2, 0.5, 0.1, 0.35, 0.5, 0.1));   // overlapping
    CHECK(canvas::circlesOverlap(0.25, 0.5, 0.125, 0.75, 0.5, 0.375));  // just touching
    CHECK(!canvas::circlesOverlap(0.2, 0.5, 0.1, 0.45, 0.5, 0.1));  // apart
    CHECK(canvas::circlesOverlap(0.5, 0.5, 0.3, 0.55, 0.5, 0.01));  // one inside the other
}

void testRectangles() {
    CHECK(canvas::rectanglesOverlap(0.2, 0.2, 0.1, 0.1, 0.25, 0.25, 0.1, 0.1));
    CHECK(canvas::rectanglesOverlap(0.25, 0.5, 0.25, 0.1, 0.75, 0.5, 0.25, 0.1));  // touching edges
    CHECK(!canvas::rectanglesOverlap(0.2, 0.2, 0.1, 0.1, 0.5, 0.2, 0.1, 0.1));   // apart in x
    CHECK(!canvas::rectanglesOverlap(0.2, 0.2, 0.1, 0.1, 0.2, 0.5, 0.1, 0.1));   // apart in y
}

void testCircleAndRectangle() {
    // A rectangle from 0.25 to 0.75 in x and 0.375 to 0.625 in y. (Values
    // that are exact in binary, so "just touching" is exact.)
    CHECK(canvas::circleOverlapsRectangle(0.5, 0.5, 0.0625, 0.5, 0.5, 0.25, 0.125));     // inside
    CHECK(canvas::circleOverlapsRectangle(0.5, 0.6875, 0.0625, 0.5, 0.5, 0.25, 0.125));  // touching the top
    CHECK(!canvas::circleOverlapsRectangle(0.5, 0.75, 0.0625, 0.5, 0.5, 0.25, 0.125));   // above it
    // Near the corner (0.75, 0.625), inside the rectangle's box widened by
    // the radius but outside the rounded corner: no overlap.
    CHECK(!canvas::circleOverlapsRectangle(0.79, 0.665, 0.05, 0.5, 0.5, 0.25, 0.125));
    CHECK(canvas::circleOverlapsRectangle(0.78, 0.655, 0.05, 0.5, 0.5, 0.25, 0.125));
}

void testDifferentScales() {
    // x from 0 to 2 and y from 0 to 1 on a square canvas: a circle of radius
    // 0.5 (in x units) is 128 pixels round, and so a quarter of the height.
    canvas::setXscale(0, 2);
    canvas::setYscale(0, 1);
    // 0.6 apart in y is 307 pixels, more than the two radii (256 pixels),
    // although in user coordinates 0.6 is less than 0.5 + 0.5.
    CHECK(!canvas::circlesOverlap(1, 0.2, 0.5, 1, 0.8, 0.5));
    CHECK(canvas::circlesOverlap(1, 0.3, 0.5, 1, 0.7, 0.5));  // 205 pixels apart
    // The same for a circle and a rectangle: the circle reaches 0.25 up.
    CHECK(!canvas::circleOverlapsRectangle(1, 0.2, 0.5, 1, 0.5, 0.5, 0.01));
    CHECK(canvas::circleOverlapsRectangle(1, 0.3, 0.5, 1, 0.5, 0.5, 0.01));
    canvas::setScale(0, 1);
}

}  // namespace

int main() {
    testDistance();
    testCircles();
    testRectangles();
    testCircleAndRectangle();
    testDifferentScales();
    return finish();
}
