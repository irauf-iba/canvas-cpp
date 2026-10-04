// Tests for turtle.hpp: positions, headings and the pen. Runs headless.

#include <canvas.hpp>
#include <turtle.hpp>

#include <cmath>

#include "check.hpp"

namespace {

bool near(double a, double b) { return std::abs(a - b) < 1e-9; }

// The colour of the canvas pixel under user coordinates (x, y), default scale.
canvas::Color pixelAt(double x, double y) {
    image::Image img = canvas::snapshot();
    int col = static_cast<int>(x * img.width);
    int row = static_cast<int>((1 - y) * img.height);
    return img[row][col];
}

void testStartAndMoves() {
    CHECK(near(turtle::x(), 0.5) && near(turtle::y(), 0.5) && turtle::heading() == 0);
    turtle::forward(0.2);
    CHECK(near(turtle::x(), 0.7) && near(turtle::y(), 0.5));
    turtle::turnLeft(90);
    CHECK(near(turtle::heading(), 90));
    turtle::forward(0.1);
    CHECK(near(turtle::x(), 0.7) && near(turtle::y(), 0.6));
    turtle::backward(0.3);
    CHECK(near(turtle::y(), 0.3));
}

void testHeadingsWrapAround() {
    turtle::setHeading(90);
    turtle::turnRight(450);  // 90 - 450 = -360
    CHECK(turtle::heading() == 0);
    turtle::turnLeft(-30);
    CHECK(near(turtle::heading(), 330));
    turtle::setHeading(720 + 45);
    CHECK(near(turtle::heading(), 45));
    turtle::setHeading(-90);
    CHECK(near(turtle::heading(), 270));
}

void testSquareCloses() {
    turtle::home();
    for (int i = 0; i < 4; ++i) {
        turtle::forward(0.3);
        turtle::turnLeft(90);
    }
    CHECK(near(turtle::x(), 0.5) && near(turtle::y(), 0.5) && near(turtle::heading(), 0));
}

void testPen() {
    canvas::clear();
    canvas::setPenWidth(4);
    turtle::moveTo(0.1, 0.1);  // pen down: draws from wherever the turtle was
    turtle::penUp();
    turtle::moveTo(0.1, 0.9);
    CHECK(near(turtle::x(), 0.1) && near(turtle::y(), 0.9));
    CHECK(pixelAt(0.1, 0.5) == canvas::WHITE);  // nothing drawn with the pen up
    turtle::penDown();
    canvas::setPenColor(canvas::RED);           // the turtle uses the canvas pen
    turtle::setHeading(0);
    turtle::forward(0.8);
    CHECK(pixelAt(0.5, 0.9) == canvas::RED);
}

void testHomeFollowsScale() {
    canvas::setScale(-1, 1);
    turtle::forward(0.5);
    turtle::home();
    CHECK(near(turtle::x(), 0) && near(turtle::y(), 0) && turtle::heading() == 0);
}

}  // namespace

int main() {
    testStartAndMoves();
    testHeadingsWrapAround();
    testSquareCloses();
    testPen();
    testHomeFollowsScale();
    return finish();
}
