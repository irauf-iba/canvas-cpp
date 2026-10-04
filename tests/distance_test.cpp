// Tests for distance(). (isMouseOver is tested in input_test, which can move
// the mouse.)

#include <canvas.hpp>

#include <cmath>

#include "check.hpp"

int main() {
    CHECK(std::abs(canvas::distance(0, 0, 3, 4) - 5) < 1e-12);
    CHECK(std::abs(canvas::distance(1, 1, 1, 1)) < 1e-12);
    CHECK(std::abs(canvas::distance(-1, 2, 2, -2) - 5) < 1e-12);
    return finish();
}
