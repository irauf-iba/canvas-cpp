// color.hpp - the Color type and predefined colors, shared by canvas and image.
//
// Color is defined in namespace canvas and is also available in namespace image,
// so canvas::Color and image::Color are the same type, as are canvas::RED and
// image::RED. Usually included through canvas.hpp or image.hpp.

#ifndef CANVAS_COLOR_HPP
#define CANVAS_COLOR_HPP

#include <cstdint>

namespace canvas {

// A color with red, green, blue and alpha (opacity) components, each 0-255.
// Alpha 255 is fully opaque, 0 fully transparent.
struct Color {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;
};

constexpr bool operator==(Color x, Color y) {
    return x.r == y.r && x.g == y.g && x.b == y.b && x.a == y.a;
}
constexpr bool operator!=(Color x, Color y) { return !(x == y); }

// Makes a color from int components. Values outside 0-255 are clamped.
// Prefer this over Color{r, g, b} when the components are int variables.
constexpr Color rgb(int r, int g, int b, int a = 255) {
    auto clamp = [](int v) { return static_cast<std::uint8_t>(v < 0 ? 0 : v > 255 ? 255 : v); };
    return Color{clamp(r), clamp(g), clamp(b), clamp(a)};
}

constexpr Color BLACK      {  0,   0,   0};
constexpr Color WHITE      {255, 255, 255};
constexpr Color GRAY       {128, 128, 128};
constexpr Color LIGHT_GRAY {192, 192, 192};
constexpr Color DARK_GRAY  { 64,  64,  64};
constexpr Color RED        {255,   0,   0};
constexpr Color GREEN      {  0, 255,   0};
constexpr Color BLUE       {  0,   0, 255};
constexpr Color CYAN       {  0, 255, 255};
constexpr Color MAGENTA    {255,   0, 255};
constexpr Color YELLOW     {255, 255,   0};
constexpr Color ORANGE     {255, 200,   0};
constexpr Color PINK       {255, 175, 175};
constexpr Color BROWN      {139,  69,  19};
constexpr Color PURPLE     {128,   0, 128};

// Colors used in the Sedgewick & Wayne textbook figures.
constexpr Color BOOK_BLUE       {  9,  90, 166};
constexpr Color BOOK_LIGHT_BLUE {103, 198, 243};
constexpr Color BOOK_RED        {150,  35,  31};

}  // namespace canvas

namespace image {

using canvas::Color;
using canvas::rgb;
using canvas::operator==;
using canvas::operator!=;

using canvas::BLACK;
using canvas::WHITE;
using canvas::GRAY;
using canvas::LIGHT_GRAY;
using canvas::DARK_GRAY;
using canvas::RED;
using canvas::GREEN;
using canvas::BLUE;
using canvas::CYAN;
using canvas::MAGENTA;
using canvas::YELLOW;
using canvas::ORANGE;
using canvas::PINK;
using canvas::BROWN;
using canvas::PURPLE;
using canvas::BOOK_BLUE;
using canvas::BOOK_LIGHT_BLUE;
using canvas::BOOK_RED;

}  // namespace image

#endif  // CANVAS_COLOR_HPP
