// canvas_impl.hpp - shared by the source files of the canvas module; not
// part of the API.
//
// The canvas is split by topic:
//     canvas.cpp   the drawing functions (window size, scale, pen, shapes,
//                  pictures, clear, save)
//     window.cpp   the window, events, frames (show, pause), animation,
//                  mouse and keyboard, debug keys, recording
//     raster.cpp   turning shapes and images into pixels
//     text.cpp     fonts and text
//     debug.cpp    hints, the grid and watched values
// Everything they share is in namespace canvas::impl, declared here.

#ifndef CANVAS_IMPL_HPP
#define CANVAS_IMPL_HPP

#include "canvas.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "internal.hpp"

namespace canvas::impl {

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

constexpr int kKeyBufferSize = 16;
constexpr int kKeyCount = static_cast<int>(Key::Alt) + 1;
constexpr Uint64 kPresentInterval = 16'666'667;  // ns; immediate mode shows at most ~60 frames/s
constexpr Uint64 kPumpInterval = 50'000'000;     // ns; keeps the window responsive in long frames
constexpr Uint64 kInputPumpInterval = 1'000'000;  // ns; input queries ask the OS at most this often
constexpr double kCurveTolerance = 0.1;          // max distance in pixels from a curve to its polygon

struct Vec {
    double x, y;
};

struct State {
    bool initialized = false;
    bool headless = false;
    bool exiting = false;  // window closed: don't keep it open at exit

    // Window. width and height are logical pixels; the canvas has pw x ph
    // physical pixels, which differ on high-DPI displays.
    std::string title = "canvas";
    int width = 512, height = 512;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    float windowW = 512, windowH = 512;  // window size in SDL window coordinates

    int pw = 0, ph = 0;
    double scale = 1;  // physical pixels per logical pixel
    std::vector<std::uint8_t> pixels;

    double xmin = 0, xmax = 1, ymin = 0, ymax = 1;

    Color pen = BLACK;
    double penWidth = 2;
    double fontSize = 16;

    bool doubleBuffered = false;
    bool dirty = false;  // canvas changed since it was last shown
    Uint64 lastPresent = 0;
    Uint64 lastPump = 0;
    bool hasPaused = false;
    Uint64 pauseEnd = 0;
    Uint64 frameInterval = 0;  // ns between frames for setFrameRate(); 0 if off
    bool hasFrame = false;
    Uint64 lastFrame = 0;      // when the previous frame was due

    // Input. Clicks and typed keys belong to the current frame; they are
    // discarded when the program next calls show() or pause().
    float mouseWX = 0, mouseWY = 0;  // window coordinates
    bool clicked = false;
    std::array<char, kKeyBufferSize> keys{};
    int keyCount = 0, keyRead = 0;
    std::array<bool, kKeyCount> keyWentDown{};  // keys pressed this frame, for wasKeyPressed()

    // Debugging aids: hints, the title readout and slow motion.
    bool hints = true;              // disableHints() or CANVAS_HINTS=0 turns them off
    unsigned hintsShown = 0;        // the kinds of hint already printed
    Color background = WHITE;       // the colour of the last clear()
    const char* drawFunction = "";  // the drawing call in progress, for hints
    double drawX = 0, drawY = 0;
    bool drewOffscreen = false;
    bool checkVisible = false;      // notice whether the call changes any pixel
    bool changedPixel = false;
    Uint64 dirtySince = 0;          // when the canvas first changed after it was shown
    bool showCoordinates = false;
    bool titleStale = false;        // the mouse moved since the title was set
    Uint64 lastTitleUpdate = 0;
    int framesCounted = 0;
    Uint64 framesSince = 0;
    double fps = 0;
    int drawDelay = 0;              // ms, for setDrawDelay()

    // The overlay, drawn over the canvas when it is shown but not into it.
    double gridXStep = 0, gridYStep = 0;  // 0: no grid
    std::vector<std::pair<std::string, std::string>> watches;  // name, value, in order
    std::vector<std::uint8_t> composed;   // the canvas with the overlay drawn on it
    bool atFrameBoundary = false;  // true between consecutive show()/pause() calls

    // Debug keys: P pauses at the next frame, N steps one frame, G toggles the grid.
    bool debugKeys = false;
    bool debugPaused = false;
    bool debugStep = false;  // N while paused: run until the next frame

    // Recording, for startRecording(): the time in the recording, which
    // follows pause() and the frame rate rather than the clock, so a slow or
    // headless run records at the intended speed.
    bool recordStarted = false;  // a frame has been recorded
    double recordClock = 0;      // seconds: when the last recorded frame appears
    double recordNext = -1;      // how long that frame is shown; < 0: measure it
    Uint64 recordTicks = 0;      // when that frame was shown

    std::map<std::string, image::Image> pictures;  // picture() files, by name
    std::vector<float> scratch;     // coverage accumulation for filled shapes
    std::vector<float> strokeMask;  // kept all zero between strokes
};

// Allocated once and never freed, so it outlives the exit handlers that still
// use it (see "Objects that are never freed" in internal.hpp).
inline State& st() {
    static State* s = new State;
    return *s;
}

// Errors: "canvas: message", and exit (canvas.cpp).
[[noreturn]] void fail(const std::string& message);
void checkFinite(const char* function, std::initializer_list<double> values);
void checkNonNegative(const char* function, const char* name, double v);

// ---------------------------------------------------------------------------
// window.cpp
// ---------------------------------------------------------------------------

// Opens the window (or sets up headless mode) on first use.
void ensureInit();

// For setCanvasSize(): the window and canvas sizes, then a new, white canvas.
void sizeWindow();
void createCanvas();

// Entry points for drawing calls: beginDraw() before, afterDraw() after.
// The forms with a function and a position record them for hints; usesPen is
// false for pictures, which don't draw in the pen colour.
void beginDraw();
void beginDraw(const char* function, double x, double y, bool usesPen = true);
void beginDraw(const char* function, const std::vector<Point>& points);
void afterDraw();

// Entry point for input queries: shows pending drawing, then reads events.
void beginInput();

// Copies the canvas (with the overlay) to the screen.
void present();
void updateTitle();
void pumpIfDue(Uint64 now);
SDL_Keycode keycodeOf(Key key);

// ---------------------------------------------------------------------------
// raster.cpp
// ---------------------------------------------------------------------------

// User coordinates to physical pixels.
double toPX(double x);
double toPY(double y);
Vec toPixel(double x, double y);

// A length along x or y in pixels. Circles, squares and arcs use lengthPX for
// both directions, so they stay round and square when the scales differ.
double lengthPX(double w);
double lengthPY(double h);

// Blends color c over the pixel with opacity alpha (0-1).
inline void blend(std::uint8_t* p, double r, double g, double b, double alpha) {
    if (alpha <= 0) return;
    if (alpha > 1) alpha = 1;
    auto mix = [](double src, std::uint8_t dst, double a) {
        return static_cast<std::uint8_t>(src * a + dst * (1 - a) + 0.5);
    };
    if (p[3] == 255) {
        p[0] = mix(r, p[0], alpha);
        p[1] = mix(g, p[1], alpha);
        p[2] = mix(b, p[2], alpha);
        return;
    }
    // Canvas pixel is not opaque (only after clear() with a transparent color).
    double da = p[3] / 255.0;
    double oa = alpha + da * (1 - alpha);
    auto mixA = [&](double src, std::uint8_t dst) {
        return static_cast<std::uint8_t>((src * alpha + dst * da * (1 - alpha)) / oa + 0.5);
    };
    p[0] = mixA(r, p[0]);
    p[1] = mixA(g, p[1]);
    p[2] = mixA(b, p[2]);
    p[3] = static_cast<std::uint8_t>(oa * 255 + 0.5);
}

inline std::uint8_t* pixelAt(int x, int y) {
    State& s = st();
    return &s.pixels[(static_cast<std::size_t>(y) * static_cast<std::size_t>(s.pw) +
                      static_cast<std::size_t>(x)) * 4];
}

// Exact v / 255 for v in 0..255*255, rounded.
inline std::uint8_t div255(unsigned v) {
    v += 128;
    return static_cast<std::uint8_t>((v + (v >> 8)) >> 8);
}

// Blends the pen color into canvas pixels. Set up once per shape so that the
// per-pixel work is a few integer operations.
struct PenBlender {
    std::uint8_t* pixels = st().pixels.data();
    std::size_t stride = static_cast<std::size_t>(st().pw) * 4;
    Color c = st().pen;
    bool track = st().checkVisible;  // for the "can't be seen" hint
    bool* changed = &st().changedPixel;

    void operator()(int x, int y, double coverage) const {
        std::uint8_t* p = pixels + static_cast<std::size_t>(y) * stride + static_cast<std::size_t>(x) * 4;
        if (track) {
            const std::uint8_t before[4] = {p[0], p[1], p[2], p[3]};
            draw(p, coverage);
            if (std::memcmp(before, p, 4) != 0) *changed = true;
            return;
        }
        draw(p, coverage);
    }

    void draw(std::uint8_t* p, double coverage) const {
        unsigned a = std::min(255u, static_cast<unsigned>(coverage * c.a + 0.5));
        if (a == 0) return;
        if (p[3] != 255) {
            blend(p, c.r, c.g, c.b, a / 255.0);
        } else if (a == 255) {
            p[0] = c.r;
            p[1] = c.g;
            p[2] = c.b;
        } else {
            unsigned ia = 255 - a;
            p[0] = div255(c.r * a + p[0] * ia);
            p[1] = div255(c.g * a + p[1] * ia);
            p[2] = div255(c.b * a + p[2] * ia);
        }
    }
};

// A rectangle of pixels [x0, x1) x [y0, y1), clipped to the canvas.
struct Box {
    int x0, y0, x1, y1;
    bool empty() const { return x0 >= x1 || y0 >= y1; }
    int w() const { return x1 - x0; }
    int h() const { return y1 - y0; }
};

Box clipBox(double minX, double minY, double maxX, double maxY);

// Shapes in pixel coordinates, drawn with the pen.
void fillPolygonPX(const std::vector<Vec>& pts);
void strokePX(const std::vector<Vec>& pts, bool closed, double width);
double penWidthPX();
std::vector<Vec> ellipsePX(Vec c, double rx, double ry, double a0, double a1, bool closed);
std::vector<Vec> squarePX(double x, double y, double h);
std::vector<Vec> rectanglePX(double x, double y, double halfWidth, double halfHeight);
std::vector<Vec> pointsPX(const std::vector<Point>& points);
void checkPoints(const char* function, const std::vector<Point>& points);

// Points from separate x and y vectors, which must have the same size.
std::vector<Point> zipPoints(const char* function, const std::vector<double>& x, const std::vector<double>& y);

// Images: picture() files are read once; drawImagePX draws an image centred
// at a pixel, scaled and (optionally) turned.
const image::Image& loadPicture(const std::string& filename);
void drawImagePX(const image::Image& img, double cx, double cy, double dw, double dh);
void drawImagePX(const image::Image& img, double cx, double cy, double dw, double dh, double degrees);

// The canvas at logical size.
image::Image canvasImage();

// ---------------------------------------------------------------------------
// text.cpp
// ---------------------------------------------------------------------------

// A string rendered into an 8-bit coverage mask, with the point the text is
// positioned by (anchorX, anchorY) in mask coordinates. align: 0 = left,
// 0.5 = centre, 1 = right.
struct TextMask {
    int w = 0, h = 0;
    std::vector<std::uint8_t> alpha;
    double anchorX = 0, anchorY = 0;
};
TextMask renderText(const std::string& text, double align);

// Text at pixel (px, py) or at user coordinates (x, y); align 0 = left,
// 0.5 = centre, 1 = right, turned counterclockwise by degrees.
void drawTextPX(double px, double py, const std::string& text, double align, double degrees);
void drawText(double x, double y, const std::string& text, double align, double degrees);

// ---------------------------------------------------------------------------
// debug.cpp
// ---------------------------------------------------------------------------

// Each kind of hint is printed once per run.
enum HintKind : unsigned { kHintOffscreen = 1, kHintSameColour = 2, kHintTransparent = 4, kHintNoShow = 8 };

void hint(unsigned kind, const std::string& message);
std::string num(double v);
std::string colourName(Color c);

// Called when a drawing call leaves nothing to draw on the canvas, with its
// bounding box in pixels.
void noteOffscreen(double minX, double minY, double maxX, double maxY);

// The grid and watched values, drawn over the canvas when it is shown.
bool hasOverlay();
const std::vector<std::uint8_t>& withOverlay();
void overlayChanged();
void toggleGrid();

}  // namespace canvas::impl

#endif  // CANVAS_IMPL_HPP
