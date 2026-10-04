// color.cpp - the color functions of color.hpp that aren't constexpr.

#include "color.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include "internal.hpp"

namespace canvas {
namespace {

void checkFinite(const char* function, double a, double b, double c = 0) {
    if (!std::isfinite(a) || !std::isfinite(b) || !std::isfinite(c)) {
        canvas_internal::fail("canvas", std::string(function) + ": an argument is NaN or infinite");
    }
}

std::uint8_t toByte(double v) {  // 0 to 1, rounded to 0 to 255
    return static_cast<std::uint8_t>(std::lround(std::clamp(v, 0.0, 1.0) * 255));
}

}  // namespace

Color hsv(double hue, double saturation, double value) {
    checkFinite("hsv", hue, saturation, value);
    const double s = std::clamp(saturation, 0.0, 1.0), v = std::clamp(value, 0.0, 1.0);
    double h = std::fmod(hue, 360.0);
    if (h < 0) h += 360;
    const double sector = h / 60;  // 0 to 6: which sixth of the color wheel
    const double chroma = v * s;
    const double x = chroma * (1 - std::abs(std::fmod(sector, 2.0) - 1));
    double r = 0, g = 0, b = 0;
    switch (static_cast<int>(sector)) {
    case 0: r = chroma, g = x; break;
    case 1: r = x, g = chroma; break;
    case 2: g = chroma, b = x; break;
    case 3: g = x, b = chroma; break;
    case 4: r = x, b = chroma; break;
    default: r = chroma, b = x; break;
    }
    const double m = v - chroma;
    return Color{toByte(r + m), toByte(g + m), toByte(b + m), 255};
}

Color mix(Color a, Color b, double t) {
    checkFinite("mix", t, 0);
    t = std::clamp(t, 0.0, 1.0);
    auto channel = [t](std::uint8_t x, std::uint8_t y) {
        return static_cast<std::uint8_t>(std::lround(x + (y - x) * t));
    };
    return Color{channel(a.r, b.r), channel(a.g, b.g), channel(a.b, b.b), channel(a.a, b.a)};
}

}  // namespace canvas
