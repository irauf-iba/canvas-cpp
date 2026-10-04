// turtle.cpp - implementation of turtle.hpp. The turtle draws with
// canvas::line, so it uses the canvas pen, scale and window.

#include "turtle.hpp"

#include <cmath>
#include <string>

#include "internal.hpp"

namespace turtle {
namespace {

const double kPi = 3.14159265358979323846;

struct State {
    bool started = false;
    double x = 0, y = 0;
    double heading = 0;  // degrees, 0 to 360
    bool penDown = true;
};

State& st() {
    static State s;
    return s;
}

// The turtle starts at the centre of the canvas when it is first used, so
// that it follows a canvas::setScale() made before.
State& turtle() {
    State& s = st();
    if (!s.started) {
        canvas_internal::canvasCenter(s.x, s.y);
        s.started = true;
    }
    return s;
}

void checkFinite(const char* function, double v) {
    if (!std::isfinite(v)) {
        canvas_internal::fail("turtle", std::string(function) + ": an argument is NaN or infinite");
    }
}

double normalized(double degrees) {
    double d = std::fmod(degrees, 360.0);
    if (d < 0) d += 360;
    return d == 360 ? 0 : d;  // fmod of a tiny negative number can round up to 360
}

void moveBy(double dx, double dy) {
    State& s = turtle();
    const double nx = s.x + dx, ny = s.y + dy;
    if (s.penDown) canvas::line(s.x, s.y, nx, ny);
    s.x = nx;
    s.y = ny;
}

}  // namespace

void forward(double step) {
    checkFinite("forward", step);
    const double a = turtle().heading * kPi / 180;
    moveBy(step * std::cos(a), step * std::sin(a));
}

void backward(double step) {
    checkFinite("backward", step);
    forward(-step);
}

void turnLeft(double degrees) {
    checkFinite("turnLeft", degrees);
    State& s = turtle();
    s.heading = normalized(s.heading + degrees);
}

void turnRight(double degrees) {
    checkFinite("turnRight", degrees);
    State& s = turtle();
    s.heading = normalized(s.heading - degrees);
}

void penUp() { turtle().penDown = false; }

void penDown() { turtle().penDown = true; }

void moveTo(double x, double y) {
    checkFinite("moveTo", x);
    checkFinite("moveTo", y);
    State& s = turtle();
    moveBy(x - s.x, y - s.y);
    s.x = x;  // exactly, without rounding from the subtraction
    s.y = y;
}

void setHeading(double degrees) {
    checkFinite("setHeading", degrees);
    turtle().heading = normalized(degrees);
}

double x() { return turtle().x; }

double y() { return turtle().y; }

double heading() { return turtle().heading; }

void home() {
    State& s = turtle();
    canvas_internal::canvasCenter(s.x, s.y);
    s.heading = 0;
}

}  // namespace turtle
