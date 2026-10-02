// color.hpp - the Color type and predefined colors, shared by draw and image.
//
// Color is defined in namespace draw and is also available in namespace image,
// so draw::Color and image::Color are the same type, as are draw::RED and
// image::RED. Usually included through draw.hpp or image.hpp.

#ifndef DRAW_COLOR_HPP
#define DRAW_COLOR_HPP

#include <cstdint>

namespace draw {

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

}  // namespace draw

namespace image {

using draw::Color;
using draw::rgb;
using draw::operator==;
using draw::operator!=;

using draw::BLACK;
using draw::WHITE;
using draw::GRAY;
using draw::LIGHT_GRAY;
using draw::DARK_GRAY;
using draw::RED;
using draw::GREEN;
using draw::BLUE;
using draw::CYAN;
using draw::MAGENTA;
using draw::YELLOW;
using draw::ORANGE;
using draw::PINK;
using draw::BROWN;
using draw::PURPLE;
using draw::BOOK_BLUE;
using draw::BOOK_LIGHT_BLUE;
using draw::BOOK_RED;

}  // namespace image

#endif  // DRAW_COLOR_HPP
