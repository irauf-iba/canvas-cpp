// draw.hpp - simple 2D drawing for introductory programming.
//
// Inspired by Princeton's StdDraw (https://introcs.cs.princeton.edu/java/stdlib/),
// with a few deliberate simplifications. Everything is a free function in
// namespace draw; there are no classes to construct.
//
//     #include <draw.hpp>
//
//     int main() {
//         draw::setPenColor(draw::BLUE);
//         draw::filledCircle(0.5, 0.5, 0.25);
//     }
//
// The window opens on the first drawing call and stays open after main()
// returns, until the user closes it. Closing the window ends the program.
//
// Coordinates: by default (0, 0) is the lower-left corner and (1, 1) the
// upper-right corner, with y pointing up. Change this with setXscale(),
// setYscale() or setScale(). Pen widths and font sizes are in screen pixels
// and do not change with the scale.
//
// Circles, squares and arcs are always round and square, even on a
// rectangular canvas or when the x and y scales differ: their size is
// measured in x units and used for both directions. Use ellipse() and
// rectangle() for shapes with separate width and height.
//
// Errors, such as a missing image file, print a message and stop the program.
//
// Headless mode: if the environment variable DRAW_HEADLESS is set to 1, no
// window is opened, pause() returns immediately and no input is ever reported.
// Drawing still works and save() writes the canvas, which is useful for
// automated grading.

#ifndef DRAW_HPP
#define DRAW_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace draw {

// ---------------------------------------------------------------------------
// Basic types
// ---------------------------------------------------------------------------

// A color with red, green, blue and alpha (opacity) components, each 0-255.
// Alpha 255 is fully opaque, 0 fully transparent.
struct Color {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;
};

// Makes a color from int components. Values outside 0-255 are clamped.
// Prefer this over Color{r, g, b} when the components are int variables.
constexpr Color rgb(int r, int g, int b, int a = 255) {
    auto clamp = [](int v) { return static_cast<std::uint8_t>(v < 0 ? 0 : v > 255 ? 255 : v); };
    return Color{clamp(r), clamp(g), clamp(b), clamp(a)};
}

// A position in user coordinates.
struct Point {
    double x = 0;
    double y = 0;
};

// Keys that can be tested with isKeyPressed().
enum class Key {
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    Space, Enter, Escape, Backspace, Tab,
    Left, Right, Up, Down,
    Shift, Control, Alt,
};

// ---------------------------------------------------------------------------
// Predefined colors
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Window and coordinates
// ---------------------------------------------------------------------------

// Sets the drawing area to width x height pixels (default 512 x 512) and
// clears it. Usually called once, before drawing anything.
void setCanvasSize(int width, int height);

// Sets the window title (default "draw").
void setTitle(const std::string& title);

// Sets the range of x or y coordinates shown in the window (default 0 to 1).
void setXscale(double min, double max);
void setYscale(double min, double max);

// Sets both the x and y ranges to min..max.
void setScale(double min, double max);

// ---------------------------------------------------------------------------
// Pen and font
// ---------------------------------------------------------------------------

// The color used by all drawing functions (default BLACK).
void setPenColor(Color color);
void setPenColor(int r, int g, int b);
Color penColor();

// The width in pixels of lines, outlines and points (default 2).
void setPenWidth(double pixels);
double penWidth();

// Text uses a built-in font unless setFont() loads a TrueType (.ttf) file.
void setFont(const std::string& ttfFile);

// The text height in pixels (default 16).
void setFontSize(double pixels);

// ---------------------------------------------------------------------------
// Shapes
//
// Shapes are positioned by their center. Outline shapes use the pen width;
// filled shapes are filled with the pen color. A radius or halfLength is
// measured in x units (see the note on coordinates at the top).
// ---------------------------------------------------------------------------

// A dot of the pen's width.
void point(double x, double y);

void line(double x0, double y0, double x1, double y1);

void circle(double x, double y, double radius);
void filledCircle(double x, double y, double radius);

// An ellipse with the given half-width and half-height.
void ellipse(double x, double y, double halfWidth, double halfHeight);
void filledEllipse(double x, double y, double halfWidth, double halfHeight);

// A circular arc from angle1 to angle2, in degrees, counterclockwise from
// the positive x-axis.
void arc(double x, double y, double radius, double angle1, double angle2);

// A square with sides of length 2 * halfLength.
void square(double x, double y, double halfLength);
void filledSquare(double x, double y, double halfLength);

// A rectangle with sides of length 2 * halfWidth and 2 * halfHeight.
void rectangle(double x, double y, double halfWidth, double halfHeight);
void filledRectangle(double x, double y, double halfWidth, double halfHeight);

// A closed polygon through the given vertices, e.g.
//     draw::polygon({{0.1, 0.1}, {0.5, 0.9}, {0.9, 0.1}});
void polygon(const std::vector<Point>& vertices);
void filledPolygon(const std::vector<Point>& vertices);

// The same, with the x and y coordinates in separate vectors of equal size.
void polygon(const std::vector<double>& x, const std::vector<double>& y);
void filledPolygon(const std::vector<double>& x, const std::vector<double>& y);

// Connected line segments through the given points (not closed), e.g. for
// plotting a function.
void polyline(const std::vector<Point>& points);
void polyline(const std::vector<double>& x, const std::vector<double>& y);

// ---------------------------------------------------------------------------
// Text and images
// ---------------------------------------------------------------------------

// Writes text centered at (x, y), or starting / ending at (x, y).
void text(double x, double y, const std::string& s);
void textLeft(double x, double y, const std::string& s);
void textRight(double x, double y, const std::string& s);

// Writes text centered at (x, y), rotated counterclockwise by degrees.
void text(double x, double y, const std::string& s, double degrees);

// Draws an image file (.png, .jpg, .bmp, .gif) centered at (x, y), at its
// natural size or scaled to the given width and height in user coordinates.
void picture(double x, double y, const std::string& filename);
void picture(double x, double y, const std::string& filename, double width, double height);

// ---------------------------------------------------------------------------
// Clearing, animation and saving
// ---------------------------------------------------------------------------

// Fills the whole canvas with the color (default WHITE).
void clear();
void clear(Color color);

// Normally each drawing call appears on screen right away. For smooth
// animation, enable double buffering: drawing then happens off screen and
// only appears when show() is called.
//
//     draw::enableDoubleBuffering();
//     while (true) {
//         draw::clear();
//         ...draw the next frame...
//         draw::show();
//         draw::pause(16);   // about 60 frames per second
//     }
void enableDoubleBuffering();
void disableDoubleBuffering();

// Copies the canvas to the screen.
void show();

// Waits for ms milliseconds. In an animation loop, the time spent drawing
// since the previous pause() counts toward the wait, so frames stay evenly
// spaced even when drawing takes a while.
void pause(int ms);

// Saves the canvas to an image file. The format comes from the extension:
// .png, .jpg or .bmp.
void save(const std::string& filename);

// ---------------------------------------------------------------------------
// Mouse
// ---------------------------------------------------------------------------

// The mouse position in user coordinates.
double mouseX();
double mouseY();

// True while any mouse button is held down.
bool isMousePressed();

// True if a mouse button was clicked during the current frame, that is, since
// the last show() or pause(). Returns true only once per click; clicks from
// earlier frames are forgotten. Use mouseX() and mouseY() for the position.
bool mouseClicked();

// ---------------------------------------------------------------------------
// Keyboard
// ---------------------------------------------------------------------------

// Characters typed during the current frame, that is, since the last show()
// or pause(). hasNextKeyTyped() is true while one is waiting; nextKeyTyped()
// removes the oldest one and returns it. Characters from earlier frames are
// forgotten, and at most 16 unread characters are kept (any more, e.g. from
// pasting, are ignored). Only ASCII characters are reported.
//
//     if (draw::hasNextKeyTyped()) ...      // handle one key per frame
//     while (draw::hasNextKeyTyped()) ...   // handle every key this frame
bool hasNextKeyTyped();
char nextKeyTyped();

// True while the key is held down.
bool isKeyPressed(Key key);

}  // namespace draw

#endif  // DRAW_HPP
