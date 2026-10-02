// draw.cpp - implementation of draw.hpp.
//
// Every drawing call is rasterized immediately into an RGBA canvas in memory,
// on the caller's thread. SDL is used only to show that canvas in a window and
// to read mouse and keyboard input; there is no render thread and no list of
// shapes.
//
// Rasterization works in physical pixels, where pixel (i, j) covers the square
// [i, i+1] x [j, j+1] and y points down. Filled shapes use exact-area coverage
// accumulation (as in font-rs); strokes use distance to the line segments, which
// gives round joins and caps and overlaps without seams.

#include "draw.hpp"

#include <SDL3/SDL.h>
// Only for SDL_SetMainReady(): the student writes an ordinary main().
#define SDL_MAIN_HANDLED
#define SDL_MAIN_NOIMPL
#include <SDL3/SDL_main.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "internal.hpp"

// stb_truetype, compiled into this file with internal linkage so it cannot
// clash with a copy of stb in the student's own program. Image files are
// read and written in image.cpp.
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace draw {
namespace {

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

const unsigned char kBuiltinFont[] = {
#include "font_data.inc"
};

constexpr int kKeyBufferSize = 16;
constexpr Uint64 kPresentInterval = 16'666'667;  // ns; immediate mode shows at most ~60 frames/s
constexpr Uint64 kPumpInterval = 50'000'000;     // ns; keeps the window responsive in long frames
constexpr Uint64 kInputPumpInterval = 1'000'000;  // ns; input queries ask the OS at most this often
constexpr double kCurveTolerance = 0.1;          // max distance in pixels from a curve to its polygon

struct Vec {
    double x, y;
};

struct Glyph {
    int w = 0, h = 0, xoff = 0, yoff = 0;
    std::vector<std::uint8_t> alpha;
};

struct Font {
    std::vector<unsigned char> file;  // empty for the built-in font
    stbtt_fontinfo info{};
    std::map<std::tuple<int, int, int>, Glyph> glyphs;  // (glyph, size * 64, subpixel x)
};

struct State {
    bool initialized = false;
    bool headless = false;
    bool exiting = false;  // window closed: don't keep it open at exit

    // Window. width and height are logical pixels; the canvas has pw x ph
    // physical pixels, which differ on high-DPI displays.
    std::string title = "draw";
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
    Font font;
    bool fontReady = false;

    bool doubleBuffered = false;
    bool dirty = false;  // canvas changed since it was last shown
    Uint64 lastPresent = 0;
    Uint64 lastPump = 0;
    bool hasPaused = false;
    Uint64 pauseEnd = 0;

    // Input. Clicks and typed keys belong to the current frame; they are
    // discarded when the program next calls show() or pause().
    float mouseWX = 0, mouseWY = 0;  // window coordinates
    bool clicked = false;
    std::array<char, kKeyBufferSize> keys{};
    int keyCount = 0, keyRead = 0;
    bool atFrameBoundary = false;  // true between consecutive show()/pause() calls

    std::map<std::string, image::Image> pictures;  // picture() files, by name
    std::vector<float> scratch;     // coverage accumulation for filled shapes
    std::vector<float> strokeMask;  // kept all zero between strokes
};

// Allocated once and never freed, so it outlives the atexit handler.
State& st() {
    static State* s = new State;
    return *s;
}

[[noreturn]] void fail(const std::string& message) { draw_internal::fail("draw", message); }

void checkFinite(const char* function, std::initializer_list<double> values) {
    for (double v : values) {
        if (!std::isfinite(v)) fail(std::string(function) + ": an argument is NaN or infinite");
    }
}

void checkNonNegative(const char* function, const char* name, double v) {
    if (v < 0) fail(std::string(function) + ": " + name + " must not be negative");
}

// ---------------------------------------------------------------------------
// Window, presentation and events
// ---------------------------------------------------------------------------

void onExit();
void present();

void sizeWindow() {
    State& s = st();
    double displayScale = 1;
    float density = 1;
    if (s.window) {
        float ds = SDL_GetWindowDisplayScale(s.window);
        float pd = SDL_GetWindowPixelDensity(s.window);
        if (ds > 0) displayScale = ds;
        if (pd > 0) density = pd;
    }
    s.pw = std::max(1, static_cast<int>(std::lround(s.width * displayScale)));
    s.ph = std::max(1, static_cast<int>(std::lround(s.height * displayScale)));
    s.scale = static_cast<double>(s.pw) / s.width;
    s.windowW = static_cast<float>(s.pw) / density;
    s.windowH = static_cast<float>(s.ph) / density;
    if (s.window) {
        SDL_SetWindowSize(s.window, static_cast<int>(std::lround(s.windowW)),
                          static_cast<int>(std::lround(s.windowH)));
        SDL_SyncWindow(s.window);
    }
}

void createCanvas() {
    State& s = st();
    s.pixels.assign(static_cast<std::size_t>(s.pw) * static_cast<std::size_t>(s.ph) * 4, 255);
    s.dirty = true;
    if (!s.renderer) return;
    if (s.texture) SDL_DestroyTexture(s.texture);
    s.texture = SDL_CreateTexture(s.renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
                                  s.pw, s.ph);
    if (!s.texture) fail(std::string("cannot create the canvas texture: ") + SDL_GetError());
    SDL_SetTextureBlendMode(s.texture, SDL_BLENDMODE_NONE);
    SDL_SetTextureScaleMode(s.texture, SDL_SCALEMODE_LINEAR);
}

void openWindow() {
    State& s = st();
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");  // let Ctrl+C stop the program
    SDL_SetMainReady();
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fail(std::string("cannot open a window: ") + SDL_GetError() +
             "\n      (set DRAW_HEADLESS=1 to draw without a window)");
    }
    s.window = SDL_CreateWindow(s.title.c_str(), s.width, s.height,
                                SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!s.window) fail(std::string("cannot open a window: ") + SDL_GetError());
    sizeWindow();
    s.renderer = SDL_CreateRenderer(s.window, nullptr);
    if (!s.renderer) fail(std::string("cannot create a renderer: ") + SDL_GetError());
    SDL_SetRenderVSync(s.renderer, s.doubleBuffered ? 1 : 0);
    SDL_ShowWindow(s.window);
    SDL_StartTextInput(s.window);
    SDL_GetMouseState(&s.mouseWX, &s.mouseWY);
}

void ensureInit() {
    State& s = st();
    if (s.initialized) return;
    s.initialized = true;
    s.headless = draw_internal::headlessRequested();
    if (s.headless) {
        sizeWindow();
    } else {
        openWindow();
    }
    createCanvas();
    present();
    std::atexit(onExit);
}

// Shows the texture as it is, e.g. after the window was uncovered.
void repaint() {
    State& s = st();
    SDL_RenderClear(s.renderer);
    SDL_RenderTexture(s.renderer, s.texture, nullptr, nullptr);
    SDL_RenderPresent(s.renderer);
}

// Copies the canvas to the screen.
void present() {
    State& s = st();
    s.dirty = false;
    if (s.headless) return;
    SDL_UpdateTexture(s.texture, nullptr, s.pixels.data(), s.pw * 4);
    repaint();
    s.lastPresent = SDL_GetTicksNS();
}

// Keys already read free their slots, so the limit applies to unread keys.
void pushKey(char c) {
    State& s = st();
    if (s.keyCount == kKeyBufferSize && s.keyRead > 0) {
        std::copy(s.keys.begin() + s.keyRead, s.keys.end(), s.keys.begin());
        s.keyCount -= s.keyRead;
        s.keyRead = 0;
    }
    if (s.keyCount < kKeyBufferSize) s.keys[static_cast<std::size_t>(s.keyCount++)] = c;
}

void handleEvent(const SDL_Event& e) {
    State& s = st();
    switch (e.type) {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        s.exiting = true;
        std::exit(0);
    case SDL_EVENT_WINDOW_EXPOSED:
        repaint();
        break;
    case SDL_EVENT_MOUSE_MOTION:
        s.mouseWX = e.motion.x;
        s.mouseWY = e.motion.y;
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        s.mouseWX = e.button.x;
        s.mouseWY = e.button.y;
        s.clicked = true;
        break;
    case SDL_EVENT_TEXT_INPUT:
        for (const char* p = e.text.text; *p; ++p) {
            if (*p >= 32 && *p < 127) pushKey(*p);  // ASCII only; UTF-8 bytes are >= 128
        }
        break;
    case SDL_EVENT_KEY_DOWN:
        // Keys that type a character but produce no text input event.
        switch (e.key.key) {
        case SDLK_RETURN:
        case SDLK_KP_ENTER: pushKey('\n'); break;
        case SDLK_BACKSPACE: pushKey('\b'); break;
        case SDLK_TAB: pushKey('\t'); break;
        case SDLK_ESCAPE: pushKey(27); break;
        case SDLK_DELETE: pushKey(127); break;
        default: break;
        }
        break;
    default:
        break;
    }
}

// Handles queued events. Asking the OS for new events is rate-limited, so
// that input queries stay cheap when called very often (e.g. once per audio
// sample); events already in SDL's queue are always handled.
void pollEvents() {
    State& s = st();
    if (s.headless) return;
    Uint64 now = SDL_GetTicksNS();
    if (now - s.lastPump >= kInputPumpInterval) {
        SDL_PumpEvents();
        s.lastPump = now;
    }
    SDL_Event e;
    while (SDL_PeepEvents(&e, 1, SDL_GETEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST) > 0) handleEvent(e);
}

// Lets the OS know the window is alive, but leaves input events queued for
// the next query, show() or pause(). Ends the program if the window was closed.
void pumpIfDue(Uint64 now) {
    State& s = st();
    if (now - s.lastPump < kPumpInterval) return;
    SDL_PumpEvents();
    s.lastPump = now;
    if (SDL_HasEvent(SDL_EVENT_QUIT)) {
        s.exiting = true;
        std::exit(0);
    }
}

// Called after every drawing operation.
void afterDraw() {
    State& s = st();
    s.dirty = true;
    if (s.headless) return;
    Uint64 now = SDL_GetTicksNS();
    if (!s.doubleBuffered && now - s.lastPresent >= kPresentInterval) present();
    pumpIfDue(now);
}

// Entry point for drawing functions.
void beginDraw() {
    ensureInit();
    st().atFrameBoundary = false;
}

// Entry point for input queries: show pending drawing, then read events.
void beginInput() {
    beginDraw();
    State& s = st();
    if (s.headless) return;
    if (!s.doubleBuffered && s.dirty && SDL_GetTicksNS() - s.lastPresent >= kPresentInterval) {
        present();
    }
    pollEvents();
}

// show() and pause() end a frame. Input from that frame is discarded, unless
// the previous call was also show() or pause() (e.g. show() then pause()), in
// which case the events read in between belong to the next frame.
void endFrame() {
    ensureInit();
    State& s = st();
    if (s.atFrameBoundary) return;
    s.atFrameBoundary = true;
    s.clicked = false;
    s.keyCount = 0;
    s.keyRead = 0;
}

void onExit() {
    State& s = st();
    if (!s.window) return;
    if (!s.exiting && !draw_internal::failing()) {
        // Keep the final picture on screen until the user closes the window.
        present();
        SDL_Event e;
        while (SDL_WaitEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT || e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) break;
            if (e.type == SDL_EVENT_WINDOW_EXPOSED) repaint();
        }
    }
    // Only video: the audio module may still be finishing its sound.
    SDL_DestroyWindow(s.window);
    s.window = nullptr;
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

// ---------------------------------------------------------------------------
// Coordinates and compositing
// ---------------------------------------------------------------------------

double toPX(double x) {
    const State& s = st();
    return (x - s.xmin) / (s.xmax - s.xmin) * s.pw;
}

double toPY(double y) {
    const State& s = st();
    return (s.ymax - y) / (s.ymax - s.ymin) * s.ph;
}

// Converts a length along x or y from user coordinates to pixels. Circles,
// squares and arcs use lengthPX for both directions so they stay round and
// square when the x and y scales differ.
double lengthPX(double w) {
    const State& s = st();
    return std::abs(w / (s.xmax - s.xmin) * s.pw);
}

double lengthPY(double h) {
    const State& s = st();
    return std::abs(h / (s.ymax - s.ymin) * s.ph);
}

Vec toPixel(double x, double y) { return {toPX(x), toPY(y)}; }

// Blends color c over the pixel with opacity alpha (0-1).
void blend(std::uint8_t* p, double r, double g, double b, double alpha) {
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

std::uint8_t* pixelAt(int x, int y) {
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

    void operator()(int x, int y, double coverage) const {
        std::uint8_t* p = pixels + static_cast<std::size_t>(y) * stride + static_cast<std::size_t>(x) * 4;
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

Box clipBox(double minX, double minY, double maxX, double maxY) {
    const State& s = st();
    auto lo = [](double v, int limit) {
        return static_cast<int>(std::clamp(std::floor(v), 0.0, static_cast<double>(limit)));
    };
    auto hi = [](double v, int limit) {
        return static_cast<int>(std::clamp(std::ceil(v), 0.0, static_cast<double>(limit)));
    };
    return {lo(minX, s.pw), lo(minY, s.ph), hi(maxX, s.pw), hi(maxY, s.ph)};
}

// ---------------------------------------------------------------------------
// Filled polygons: exact-area coverage accumulation
// ---------------------------------------------------------------------------

// Adds the signed area contribution of the line p0-p1 to acc, a buffer of h
// rows of `stride` floats. x must lie within [0, stride - 2].
void accumulateLine(float* acc, int stride, int h, Vec p0, Vec p1) {
    if (p0.y == p1.y) return;
    double dir = 1;
    if (p0.y > p1.y) {
        std::swap(p0, p1);
        dir = -1;
    }
    const double maxX = stride - 2;
    double dxdy = (p1.x - p0.x) / (p1.y - p0.y);
    double x = p0.x;
    if (p0.y < 0) x -= p0.y * dxdy;
    int yStart = static_cast<int>(std::clamp(std::floor(p0.y), 0.0, static_cast<double>(h)));
    int yEnd = static_cast<int>(std::clamp(std::ceil(p1.y), 0.0, static_cast<double>(h)));
    for (int y = yStart; y < yEnd; ++y) {
        double dy = std::min(y + 1.0, p1.y) - std::max(static_cast<double>(y), p0.y);
        double xNext = x + dxdy * dy;
        double d = dy * dir;
        double x0 = std::clamp(std::min(x, xNext), 0.0, maxX);
        double x1 = std::clamp(std::max(x, xNext), 0.0, maxX);
        float* row = acc + static_cast<std::ptrdiff_t>(y) * stride;
        double x0Floor = std::floor(x0);
        int x0i = static_cast<int>(x0Floor);
        int x1i = static_cast<int>(std::ceil(x1));
        if (x1i <= x0i + 1) {
            // The line stays within one pixel column on this row.
            double xm = 0.5 * (x0 + x1) - x0Floor;
            row[x0i] += static_cast<float>(d - d * xm);
            row[x0i + 1] += static_cast<float>(d * xm);
        } else {
            double inv = 1.0 / (x1 - x0);
            double x0f = x0 - x0Floor;
            double a0 = 0.5 * inv * (1 - x0f) * (1 - x0f);
            double x1f = x1 - x1i + 1;
            double am = 0.5 * inv * x1f * x1f;
            row[x0i] += static_cast<float>(d * a0);
            if (x1i == x0i + 2) {
                row[x0i + 1] += static_cast<float>(d * (1 - a0 - am));
            } else {
                double a1 = inv * (1.5 - x0f);
                row[x0i + 1] += static_cast<float>(d * (a1 - a0));
                for (int xi = x0i + 2; xi < x1i - 1; ++xi) row[xi] += static_cast<float>(d * inv);
                double a2 = a1 + (x1i - x0i - 3) * inv;
                row[x1i - 1] += static_cast<float>(d * (1 - a2 - am));
            }
            row[x1i] += static_cast<float>(d * am);
        }
        x = xNext;
    }
}

// Fills a closed polygon (pixel coordinates) with the pen color, using the
// nonzero rule.
void fillPolygonPX(const std::vector<Vec>& pts) {
    if (pts.size() < 3) return;
    double minX = pts[0].x, maxX = minX, minY = pts[0].y, maxY = minY;
    for (const Vec& p : pts) {
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
    }
    Box box = clipBox(minX, minY, maxX, maxY);
    if (box.empty()) return;

    const int w = box.w(), h = box.h(), stride = w + 2;
    std::vector<float>& acc = st().scratch;
    acc.assign(static_cast<std::size_t>(stride) * static_cast<std::size_t>(h), 0.0f);

    // Edges are split where they cross the left and right sides of the box;
    // the outside parts are then clamped onto those sides, which preserves
    // the winding inside the box.
    for (std::size_t i = 0; i < pts.size(); ++i) {
        Vec a{pts[i].x - box.x0, pts[i].y - box.y0};
        const Vec& next = pts[(i + 1) % pts.size()];
        Vec b{next.x - box.x0, next.y - box.y0};
        double ts[4] = {0, 0, 0, 0};
        int n = 1;
        for (double edge : {0.0, static_cast<double>(w)}) {
            if ((a.x - edge) * (b.x - edge) < 0) ts[n++] = (edge - a.x) / (b.x - a.x);
        }
        if (n == 3 && ts[1] > ts[2]) std::swap(ts[1], ts[2]);  // two crossings, in order
        ts[n++] = 1;
        for (int k = 0; k + 1 < n; ++k) {
            auto at = [&](double t) {
                return Vec{std::clamp(a.x + (b.x - a.x) * t, 0.0, static_cast<double>(w)),
                           a.y + (b.y - a.y) * t};
            };
            accumulateLine(acc.data(), stride, h, at(ts[k]), at(ts[k + 1]));
        }
    }

    PenBlender pen;
    for (int y = 0; y < h; ++y) {
        const float* row = acc.data() + static_cast<std::ptrdiff_t>(y) * stride;
        double sum = 0;
        for (int x = 0; x < w; ++x) {
            sum += row[x];
            double coverage = std::min(1.0, std::abs(sum));
            if (coverage > 1e-4) pen(box.x0 + x, box.y0 + y, coverage);
        }
    }
}

// ---------------------------------------------------------------------------
// Strokes: coverage from distance to the line segments
// ---------------------------------------------------------------------------

// Strokes connected segments through pts (pixel coordinates) with the pen
// color, `width` pixels wide, with round joins and caps. A single point gives
// a round dot.
void strokePX(const std::vector<Vec>& pts, bool closed, double width) {
    if (pts.empty()) return;
    const double r = width / 2;
    const double reach = r + 0.5;  // coverage is zero beyond this distance
    double minX = pts[0].x, maxX = minX, minY = pts[0].y, maxY = minY;
    for (const Vec& p : pts) {
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
    }
    Box box = clipBox(minX - reach, minY - reach, maxX + reach, maxY + reach);
    if (box.empty()) return;

    // The mask is all zeros between calls; only pixels near the segments are
    // written, and the second pass resets them. This keeps the cost
    // proportional to the stroke's area, not its bounding box.
    std::vector<float>& mask = st().strokeMask;
    const std::size_t area = static_cast<std::size_t>(box.w()) * static_cast<std::size_t>(box.h());
    if (mask.size() < area) mask.resize(area, 0.0f);

    const std::size_t segments = pts.size() == 1 ? 1 : (closed ? pts.size() : pts.size() - 1);

    struct Segment {
        double ax, ay, dx, dy, invLen2;  // invLen2 is 0 for a single point
    };

    // Calls visit(x, y, segment, mask) for every pixel that may be within
    // `reach` of the segment.
    auto forSegmentPixels = [&](auto visit) {
        for (std::size_t i = 0; i < segments; ++i) {
            const Vec a = pts[i];
            const Vec b = pts.size() == 1 ? a : pts[(i + 1) % pts.size()];
            const double dx = b.x - a.x, dy = b.y - a.y;
            const double len2 = dx * dx + dy * dy, len = std::sqrt(len2);
            const Segment sg{a.x, a.y, dx, dy, len2 > 0 ? 1 / len2 : 0};
            Box seg = clipBox(std::min(a.x, b.x) - reach, std::min(a.y, b.y) - reach,
                              std::max(a.x, b.x) + reach, std::max(a.y, b.y) + reach);
            for (int y = seg.y0; y < seg.y1; ++y) {
                // Only pixels within `reach` of the infinite line can be
                // covered, which limits long diagonal lines to a narrow band.
                int xs = seg.x0, xe = seg.x1;
                if (std::abs(dy) > 1e-9) {
                    double base = a.x + (y + 0.5 - a.y) * dx / dy;
                    double half = reach * len / std::abs(dy);
                    double lo = std::floor(base - half), hi = std::ceil(base + half);
                    xs = static_cast<int>(std::clamp(lo, static_cast<double>(seg.x0), static_cast<double>(seg.x1)));
                    xe = static_cast<int>(std::clamp(hi, static_cast<double>(seg.x0), static_cast<double>(seg.x1)));
                }
                float* row = mask.data() + (static_cast<std::ptrdiff_t>(y - box.y0) * box.w() + (xs - box.x0));
                for (int x = xs; x < xe; ++x, ++row) visit(x, y, sg, *row);
            }
        }
    };

    // Coverage is r + 0.5 - distance, clamped to 0..1, so pixels closer than
    // r - 0.5 are fully covered and pixels farther than r + 0.5 not at all;
    // only the edge needs a square root.
    const double inner = std::max(0.0, r - 0.5), inner2 = inner * inner;
    const double outer2 = (r + 0.5) * (r + 0.5);
    forSegmentPixels([&](int x, int y, const Segment& sg, float& m) {
        if (m >= 1) return;
        const double qx = x + 0.5 - sg.ax, qy = y + 0.5 - sg.ay;
        double t = (qx * sg.dx + qy * sg.dy) * sg.invLen2;
        t = t < 0 ? 0 : t > 1 ? 1 : t;
        const double ex = qx - t * sg.dx, ey = qy - t * sg.dy;
        const double d2 = ex * ex + ey * ey;
        if (d2 >= outer2) return;
        const float coverage = d2 <= inner2 ? 1.0f : static_cast<float>(std::min(1.0, r + 0.5 - std::sqrt(d2)));
        if (coverage > m) m = coverage;
    });
    PenBlender pen;
    forSegmentPixels([&pen](int x, int y, const Segment&, float& m) {
        if (m > 1e-4f) pen(x, y, m);
        m = 0;
    });
}

double penWidthPX() { return st().penWidth * st().scale; }

// Points on an ellipse (pixel coordinates) from angle a0 to a1 in degrees,
// counterclockwise, with enough points that the polygon stays within
// kCurveTolerance of the curve.
std::vector<Vec> ellipsePX(Vec c, double rx, double ry, double a0, double a1, bool closed) {
    const double pi = 3.14159265358979323846;
    double rmax = std::max(rx, ry);
    double step = rmax > kCurveTolerance ? 2 * std::acos(1 - kCurveTolerance / rmax) : pi / 4;
    double sweep = (a1 - a0) * pi / 180;
    int n = std::clamp(static_cast<int>(std::ceil(std::abs(sweep) / step)), 8, 4000);
    std::vector<Vec> pts;
    pts.reserve(static_cast<std::size_t>(n) + 1);
    int count = closed ? n : n + 1;
    for (int i = 0; i < count; ++i) {
        double t = a0 * pi / 180 + sweep * i / n;
        pts.push_back({c.x + rx * std::cos(t), c.y - ry * std::sin(t)});
    }
    return pts;
}

// A square centered at (x, y) with half side h pixels.
std::vector<Vec> squarePX(double x, double y, double h) {
    const Vec c = toPixel(x, y);
    return {{c.x - h, c.y + h}, {c.x + h, c.y + h}, {c.x + h, c.y - h}, {c.x - h, c.y - h}};
}

std::vector<Vec> rectanglePX(double x, double y, double halfWidth, double halfHeight) {
    return {toPixel(x - halfWidth, y - halfHeight), toPixel(x + halfWidth, y - halfHeight),
            toPixel(x + halfWidth, y + halfHeight), toPixel(x - halfWidth, y + halfHeight)};
}

std::vector<Vec> pointsPX(const std::vector<Point>& points) {
    std::vector<Vec> pts;
    pts.reserve(points.size());
    for (const Point& p : points) pts.push_back(toPixel(p.x, p.y));
    return pts;
}

std::vector<Point> zipPoints(const char* function, const std::vector<double>& x,
                             const std::vector<double>& y) {
    if (x.size() != y.size()) fail(std::string(function) + ": x and y must have the same size");
    std::vector<Point> points;
    points.reserve(x.size());
    for (std::size_t i = 0; i < x.size(); ++i) points.push_back({x[i], y[i]});
    return points;
}

void checkPoints(const char* function, const std::vector<Point>& points) {
    for (const Point& p : points) checkFinite(function, {p.x, p.y});
}

// ---------------------------------------------------------------------------
// Text
// ---------------------------------------------------------------------------

Font& currentFont() {
    State& s = st();
    if (!s.fontReady) {
        stbtt_InitFont(&s.font.info, kBuiltinFont, stbtt_GetFontOffsetForIndex(kBuiltinFont, 0));
        s.fontReady = true;
    }
    return s.font;
}

std::vector<int> decodeUtf8(const std::string& s) {
    std::vector<int> out;
    for (std::size_t i = 0; i < s.size();) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        int len = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 0;
        if (len == 0 || i + static_cast<std::size_t>(len) > s.size()) {
            out.push_back(0xFFFD);
            ++i;
            continue;
        }
        int cp = len == 1 ? c : c & (0x7F >> len);
        for (int k = 1; k < len; ++k) cp = (cp << 6) | (static_cast<unsigned char>(s[i + static_cast<std::size_t>(k)]) & 0x3F);
        out.push_back(cp);
        i += static_cast<std::size_t>(len);
    }
    return out;
}

// A string rendered into an 8-bit coverage mask, with the point the text is
// positioned by (anchorX, anchorY) in mask coordinates.
struct TextMask {
    int w = 0, h = 0;
    std::vector<std::uint8_t> alpha;
    double anchorX = 0, anchorY = 0;
};

// align: 0 = left, 0.5 = center, 1 = right.
TextMask renderText(const std::string& text, double align) {
    State& s = st();
    Font& font = currentFont();
    const stbtt_fontinfo* info = &font.info;
    const double k = stbtt_ScaleForPixelHeight(info, static_cast<float>(s.fontSize * s.scale));
    const int sizeKey = static_cast<int>(std::lround(s.fontSize * s.scale * 64));
    constexpr int kSubpixels = 4;

    int ascentU, descentU, gapU;
    stbtt_GetFontVMetrics(info, &ascentU, &descentU, &gapU);
    const double ascent = ascentU * k, descent = descentU * k;  // descent is negative

    struct Placed {
        const Glyph* glyph;
        int x, y;
    };
    std::vector<Placed> placed;
    double pen = 0;
    int prev = -1;
    int minX = 0, maxX = 0, minY = static_cast<int>(std::floor(-ascent)),
        maxY = static_cast<int>(std::ceil(-descent));
    for (int cp : decodeUtf8(text)) {
        if (cp < 32) continue;
        int g = stbtt_FindGlyphIndex(info, cp);
        if (prev >= 0) pen += stbtt_GetGlyphKernAdvance(info, prev, g) * k;
        prev = g;

        double penFloor = std::floor(pen);
        int sub = static_cast<int>((pen - penFloor) * kSubpixels);
        auto key = std::make_tuple(g, sizeKey, sub);
        auto it = font.glyphs.find(key);
        if (it == font.glyphs.end()) {
            Glyph glyph;
            float shift = static_cast<float>(sub) / kSubpixels;
            unsigned char* bitmap = stbtt_GetGlyphBitmapSubpixel(
                info, static_cast<float>(k), static_cast<float>(k), shift, 0, g, &glyph.w, &glyph.h, &glyph.xoff, &glyph.yoff);
            if (bitmap) {
                glyph.alpha.assign(bitmap, bitmap + glyph.w * glyph.h);
                stbtt_FreeBitmap(bitmap, nullptr);
            }
            it = font.glyphs.emplace(key, std::move(glyph)).first;
        }
        const Glyph& glyph = it->second;
        Placed p{&glyph, static_cast<int>(penFloor) + glyph.xoff, glyph.yoff};
        placed.push_back(p);
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x + glyph.w);
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y + glyph.h);

        int advance, lsb;
        stbtt_GetGlyphHMetrics(info, g, &advance, &lsb);
        pen += advance * k;
    }
    maxX = std::max(maxX, static_cast<int>(std::ceil(pen)));

    // Mask coordinates: the baseline origin is at (-minX, -minY).
    TextMask mask;
    mask.w = maxX - minX;
    mask.h = maxY - minY;
    mask.alpha.assign(static_cast<std::size_t>(mask.w) * static_cast<std::size_t>(mask.h), 0);
    for (const Placed& p : placed) {
        for (int gy = 0; gy < p.glyph->h; ++gy) {
            for (int gx = 0; gx < p.glyph->w; ++gx) {
                std::uint8_t a = p.glyph->alpha[static_cast<std::size_t>(gy * p.glyph->w + gx)];
                std::uint8_t& dst = mask.alpha[static_cast<std::size_t>(
                    (p.y - minY + gy) * mask.w + (p.x - minX + gx))];
                dst = std::max(dst, a);
            }
        }
    }
    // Vertically centered on the font's ascent-descent box.
    mask.anchorX = align * pen - minX;
    mask.anchorY = -minY - (ascent + descent) / 2;
    return mask;
}

// Bilinear sample of an 8-bit mask at (u, v), where pixel (i, j) has its center
// at (i + 0.5, j + 0.5). Outside the mask is 0.
double sampleMask(const TextMask& m, double u, double v) {
    u -= 0.5;
    v -= 0.5;
    int i = static_cast<int>(std::floor(u)), j = static_cast<int>(std::floor(v));
    double fu = u - i, fv = v - j;
    auto at = [&](int x, int y) -> double {
        if (x < 0 || y < 0 || x >= m.w || y >= m.h) return 0;
        return m.alpha[static_cast<std::size_t>(y * m.w + x)];
    };
    double top = at(i, j) * (1 - fu) + at(i + 1, j) * fu;
    double bottom = at(i, j + 1) * (1 - fu) + at(i + 1, j + 1) * fu;
    return (top * (1 - fv) + bottom * fv) / 255.0;
}

void drawText(double x, double y, const std::string& text, double align, double degrees) {
    TextMask m = renderText(text, align);
    if (m.w == 0 || m.h == 0) return;
    PenBlender pen;
    const double px = toPX(x), py = toPY(y);

    if (degrees == 0) {
        // Snap to whole pixels so unrotated text stays sharp.
        int left = static_cast<int>(std::lround(px - m.anchorX));
        int top = static_cast<int>(std::lround(py - m.anchorY));
        Box box = clipBox(left, top, left + m.w, top + m.h);
        for (int cy = box.y0; cy < box.y1; ++cy) {
            for (int cx = box.x0; cx < box.x1; ++cx) {
                std::uint8_t a = m.alpha[static_cast<std::size_t>((cy - top) * m.w + (cx - left))];
                if (a) pen(cx, cy, a / 255.0);
            }
        }
        return;
    }

    // Rotated counterclockwise on screen (y points down): a mask offset (u, v)
    // from the anchor lands at (px + u cos + v sin, py - u sin + v cos).
    const double t = degrees * 3.14159265358979323846 / 180;
    const double c = std::cos(t), s = std::sin(t);
    double minX = px, maxX = px, minY = py, maxY = py;
    for (double u : {-m.anchorX, m.w - m.anchorX}) {
        for (double v : {-m.anchorY, m.h - m.anchorY}) {
            double cx = px + u * c + v * s, cy = py - u * s + v * c;
            minX = std::min(minX, cx);
            maxX = std::max(maxX, cx);
            minY = std::min(minY, cy);
            maxY = std::max(maxY, cy);
        }
    }
    Box box = clipBox(minX - 1, minY - 1, maxX + 1, maxY + 1);
    for (int cy = box.y0; cy < box.y1; ++cy) {
        for (int cx = box.x0; cx < box.x1; ++cx) {
            double dx = cx + 0.5 - px, dy = cy + 0.5 - py;
            double u = dx * c - dy * s, v = dx * s + dy * c;
            double a = sampleMask(m, m.anchorX + u, m.anchorY + v);
            if (a > 1e-4) pen(cx, cy, a);
        }
    }
}

// ---------------------------------------------------------------------------
// Images
// ---------------------------------------------------------------------------

const image::Image& loadPicture(const std::string& filename) {
    State& s = st();
    auto it = s.pictures.find(filename);
    if (it != s.pictures.end()) return it->second;
    image::Image img = draw_internal::readImageFile(filename, "draw", "picture");
    return s.pictures.emplace(filename, std::move(img)).first->second;
}

// Draws the image centered at pixel (cx, cy), scaled to dw x dh pixels,
// with bilinear filtering on premultiplied colors.
void drawImagePX(const image::Image& img, double cx, double cy, double dw, double dh) {
    if (dw <= 0 || dh <= 0 || img.width == 0 || img.height == 0) return;
    const double left = std::round(cx - dw / 2), top = std::round(cy - dh / 2);
    Box box = clipBox(left, top, left + dw, top + dh);
    const double sx = img.width / dw, sy = img.height / dh;
    auto texel = [&](int x, int y) -> const Color& {
        x = std::clamp(x, 0, img.width - 1);
        y = std::clamp(y, 0, img.height - 1);
        return img.pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(img.width) +
                          static_cast<std::size_t>(x)];
    };
    for (int y = box.y0; y < box.y1; ++y) {
        double v = (y + 0.5 - top) * sy - 0.5;
        int j = static_cast<int>(std::floor(v));
        double fv = v - j;
        for (int x = box.x0; x < box.x1; ++x) {
            double u = (x + 0.5 - left) * sx - 0.5;
            int i = static_cast<int>(std::floor(u));
            double fu = u - i;
            double sum[4] = {0, 0, 0, 0};
            const Color* q[4] = {&texel(i, j), &texel(i + 1, j), &texel(i, j + 1), &texel(i + 1, j + 1)};
            const double wt[4] = {(1 - fu) * (1 - fv), fu * (1 - fv), (1 - fu) * fv, fu * fv};
            for (int n = 0; n < 4; ++n) {
                double a = wt[n] * q[n]->a;
                sum[0] += q[n]->r * a;
                sum[1] += q[n]->g * a;
                sum[2] += q[n]->b * a;
                sum[3] += a;
            }
            if (sum[3] <= 0) continue;
            blend(pixelAt(x, y), sum[0] / sum[3], sum[1] / sum[3], sum[2] / sum[3], sum[3] / 255.0);
        }
    }
}

// The canvas at logical size (box-filtered down from physical pixels on
// high-DPI displays).
image::Image canvasImage() {
    const State& s = st();
    image::Image out;
    out.width = s.width;
    out.height = s.height;
    out.pixels.resize(static_cast<std::size_t>(s.width) * static_cast<std::size_t>(s.height));
    if (s.pw == s.width && s.ph == s.height) {
        std::memcpy(out.pixels.data(), s.pixels.data(), s.pixels.size());
        return out;
    }
    const double fx = static_cast<double>(s.pw) / s.width, fy = static_cast<double>(s.ph) / s.height;
    auto range = [](int i, double f, int limit) {
        int a = static_cast<int>(std::ceil(i * f - 0.5));
        int b = static_cast<int>(std::ceil((i + 1) * f - 0.5));
        a = std::clamp(a, 0, limit - 1);
        return std::make_pair(a, std::clamp(b, a + 1, limit));
    };
    for (int y = 0; y < s.height; ++y) {
        auto [y0, y1] = range(y, fy, s.ph);
        for (int x = 0; x < s.width; ++x) {
            auto [x0, x1] = range(x, fx, s.pw);
            unsigned sum[4] = {0, 0, 0, 0};
            for (int j = y0; j < y1; ++j) {
                for (int i = x0; i < x1; ++i) {
                    const std::uint8_t* p = &s.pixels[(static_cast<std::size_t>(j) * static_cast<std::size_t>(s.pw) +
                                                       static_cast<std::size_t>(i)) * 4];
                    for (int c = 0; c < 4; ++c) sum[c] += p[c];
                }
            }
            unsigned n = static_cast<unsigned>((x1 - x0) * (y1 - y0));
            auto avg = [&](int c) { return static_cast<std::uint8_t>((sum[c] + n / 2) / n); };
            out.pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(s.width) +
                       static_cast<std::size_t>(x)] = Color{avg(0), avg(1), avg(2), avg(3)};
        }
    }
    return out;
}

// Maps a Key to the SDL keycode of the key with that label.
SDL_Keycode keycodeOf(Key key) {
    auto offset = [key](Key first) { return static_cast<SDL_Keycode>(static_cast<int>(key) - static_cast<int>(first)); };
    if (key >= Key::A && key <= Key::Z) return SDLK_A + offset(Key::A);
    if (key >= Key::Num0 && key <= Key::Num9) return SDLK_0 + offset(Key::Num0);
    switch (key) {
    case Key::Space: return SDLK_SPACE;
    case Key::Enter: return SDLK_RETURN;
    case Key::Escape: return SDLK_ESCAPE;
    case Key::Backspace: return SDLK_BACKSPACE;
    case Key::Tab: return SDLK_TAB;
    case Key::Left: return SDLK_LEFT;
    case Key::Right: return SDLK_RIGHT;
    case Key::Up: return SDLK_UP;
    case Key::Down: return SDLK_DOWN;
    case Key::Shift: return SDLK_LSHIFT;
    case Key::Control: return SDLK_LCTRL;
    case Key::Alt: return SDLK_LALT;
    default: return SDLK_UNKNOWN;
    }
}

}  // namespace

// ===========================================================================
// Public API
// ===========================================================================

// --- Window and coordinates ------------------------------------------------

void setCanvasSize(int width, int height) {
    if (width <= 0 || height <= 0) fail("setCanvasSize: width and height must be positive");
    State& s = st();
    s.width = width;
    s.height = height;
    if (!s.initialized) return;
    sizeWindow();
    createCanvas();
    afterDraw();
}

void setTitle(const std::string& title) {
    State& s = st();
    s.title = title;
    if (s.window) SDL_SetWindowTitle(s.window, title.c_str());
}

void setXscale(double min, double max) {
    checkFinite("setXscale", {min, max});
    if (min == max) fail("setXscale: min and max must be different");
    st().xmin = min;
    st().xmax = max;
}

void setYscale(double min, double max) {
    checkFinite("setYscale", {min, max});
    if (min == max) fail("setYscale: min and max must be different");
    st().ymin = min;
    st().ymax = max;
}

void setScale(double min, double max) {
    checkFinite("setScale", {min, max});
    if (min == max) fail("setScale: min and max must be different");
    setXscale(min, max);
    setYscale(min, max);
}

// --- Pen and font ------------------------------------------------------------

void setPenColor(Color color) { st().pen = color; }

void setPenColor(int r, int g, int b) { st().pen = rgb(r, g, b); }

Color penColor() { return st().pen; }

void setPenWidth(double pixels) {
    checkFinite("setPenWidth", {pixels});
    checkNonNegative("setPenWidth", "the width", pixels);
    st().penWidth = pixels;
}

double penWidth() { return st().penWidth; }

void setFont(const std::string& ttfFile) {
    std::ifstream in(ttfFile, std::ios::binary);
    if (!in) fail("setFont: cannot open '" + ttfFile + "'");
    std::vector<unsigned char> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    State& s = st();
    Font font;
    font.file = std::move(data);
    int offset = stbtt_GetFontOffsetForIndex(font.file.data(), 0);
    if (offset < 0 || !stbtt_InitFont(&font.info, font.file.data(), offset)) {
        fail("setFont: '" + ttfFile + "' is not a TrueType font");
    }
    // stbtt_fontinfo points into font.file, whose buffer survives the move.
    s.font = std::move(font);
    s.fontReady = true;
}

void setFontSize(double pixels) {
    checkFinite("setFontSize", {pixels});
    if (pixels <= 0) fail("setFontSize: the size must be positive");
    st().fontSize = pixels;
}

// --- Shapes ------------------------------------------------------------------

void point(double x, double y) {
    checkFinite("point", {x, y});
    beginDraw();
    strokePX({toPixel(x, y)}, false, std::max(1.0, penWidthPX()));
    afterDraw();
}

void line(double x0, double y0, double x1, double y1) {
    checkFinite("line", {x0, y0, x1, y1});
    beginDraw();
    strokePX({toPixel(x0, y0), toPixel(x1, y1)}, false, penWidthPX());
    afterDraw();
}

void circle(double x, double y, double radius) {
    checkFinite("circle", {x, y, radius});
    checkNonNegative("circle", "radius", radius);
    beginDraw();
    strokePX(ellipsePX(toPixel(x, y), lengthPX(radius), lengthPX(radius), 0, 360, true), true,
             penWidthPX());
    afterDraw();
}

void filledCircle(double x, double y, double radius) {
    checkFinite("filledCircle", {x, y, radius});
    checkNonNegative("filledCircle", "radius", radius);
    beginDraw();
    fillPolygonPX(ellipsePX(toPixel(x, y), lengthPX(radius), lengthPX(radius), 0, 360, true));
    afterDraw();
}

void ellipse(double x, double y, double halfWidth, double halfHeight) {
    checkFinite("ellipse", {x, y, halfWidth, halfHeight});
    checkNonNegative("ellipse", "halfWidth", halfWidth);
    checkNonNegative("ellipse", "halfHeight", halfHeight);
    beginDraw();
    strokePX(ellipsePX(toPixel(x, y), lengthPX(halfWidth), lengthPY(halfHeight), 0, 360, true),
             true, penWidthPX());
    afterDraw();
}

void filledEllipse(double x, double y, double halfWidth, double halfHeight) {
    checkFinite("filledEllipse", {x, y, halfWidth, halfHeight});
    checkNonNegative("filledEllipse", "halfWidth", halfWidth);
    checkNonNegative("filledEllipse", "halfHeight", halfHeight);
    beginDraw();
    fillPolygonPX(ellipsePX(toPixel(x, y), lengthPX(halfWidth), lengthPY(halfHeight), 0, 360, true));
    afterDraw();
}

void arc(double x, double y, double radius, double angle1, double angle2) {
    checkFinite("arc", {x, y, radius, angle1, angle2});
    checkNonNegative("arc", "radius", radius);
    while (angle2 < angle1) angle2 += 360;
    beginDraw();
    strokePX(ellipsePX(toPixel(x, y), lengthPX(radius), lengthPX(radius), angle1, angle2, false),
             false, penWidthPX());
    afterDraw();
}

void square(double x, double y, double halfLength) {
    checkFinite("square", {x, y, halfLength});
    checkNonNegative("square", "halfLength", halfLength);
    beginDraw();
    strokePX(squarePX(x, y, lengthPX(halfLength)), true, penWidthPX());
    afterDraw();
}

void filledSquare(double x, double y, double halfLength) {
    checkFinite("filledSquare", {x, y, halfLength});
    checkNonNegative("filledSquare", "halfLength", halfLength);
    beginDraw();
    fillPolygonPX(squarePX(x, y, lengthPX(halfLength)));
    afterDraw();
}

void rectangle(double x, double y, double halfWidth, double halfHeight) {
    checkFinite("rectangle", {x, y, halfWidth, halfHeight});
    checkNonNegative("rectangle", "halfWidth", halfWidth);
    checkNonNegative("rectangle", "halfHeight", halfHeight);
    beginDraw();
    strokePX(rectanglePX(x, y, halfWidth, halfHeight), true, penWidthPX());
    afterDraw();
}

void filledRectangle(double x, double y, double halfWidth, double halfHeight) {
    checkFinite("filledRectangle", {x, y, halfWidth, halfHeight});
    checkNonNegative("filledRectangle", "halfWidth", halfWidth);
    checkNonNegative("filledRectangle", "halfHeight", halfHeight);
    beginDraw();
    fillPolygonPX(rectanglePX(x, y, halfWidth, halfHeight));
    afterDraw();
}

void polygon(const std::vector<Point>& vertices) {
    checkPoints("polygon", vertices);
    beginDraw();
    strokePX(pointsPX(vertices), true, penWidthPX());
    afterDraw();
}

void filledPolygon(const std::vector<Point>& vertices) {
    checkPoints("filledPolygon", vertices);
    beginDraw();
    fillPolygonPX(pointsPX(vertices));
    afterDraw();
}

void polygon(const std::vector<double>& x, const std::vector<double>& y) {
    polygon(zipPoints("polygon", x, y));
}

void filledPolygon(const std::vector<double>& x, const std::vector<double>& y) {
    filledPolygon(zipPoints("filledPolygon", x, y));
}

void polyline(const std::vector<Point>& points) {
    checkPoints("polyline", points);
    beginDraw();
    strokePX(pointsPX(points), false, penWidthPX());
    afterDraw();
}

void polyline(const std::vector<double>& x, const std::vector<double>& y) {
    polyline(zipPoints("polyline", x, y));
}

// --- Text and images ---------------------------------------------------------

void text(double x, double y, const std::string& s) {
    checkFinite("text", {x, y});
    beginDraw();
    drawText(x, y, s, 0.5, 0);
    afterDraw();
}

void textLeft(double x, double y, const std::string& s) {
    checkFinite("textLeft", {x, y});
    beginDraw();
    drawText(x, y, s, 0, 0);
    afterDraw();
}

void textRight(double x, double y, const std::string& s) {
    checkFinite("textRight", {x, y});
    beginDraw();
    drawText(x, y, s, 1, 0);
    afterDraw();
}

void text(double x, double y, const std::string& s, double degrees) {
    checkFinite("text", {x, y, degrees});
    beginDraw();
    drawText(x, y, s, 0.5, std::fmod(degrees, 360.0));
    afterDraw();
}

void picture(double x, double y, const image::Image& img) {
    checkFinite("picture", {x, y});
    draw_internal::checkImage(img, "draw", "picture");
    beginDraw();
    drawImagePX(img, toPX(x), toPY(y), img.width * st().scale, img.height * st().scale);
    afterDraw();
}

void picture(double x, double y, const image::Image& img, double width, double height) {
    checkFinite("picture", {x, y, width, height});
    checkNonNegative("picture", "width", width);
    checkNonNegative("picture", "height", height);
    draw_internal::checkImage(img, "draw", "picture");
    beginDraw();
    drawImagePX(img, toPX(x), toPY(y), lengthPX(width), lengthPY(height));
    afterDraw();
}

void picture(double x, double y, const std::string& filename) {
    picture(x, y, loadPicture(filename));
}

void picture(double x, double y, const std::string& filename, double width, double height) {
    picture(x, y, loadPicture(filename), width, height);
}

// --- Clearing, animation and saving ------------------------------------------

void clear() { clear(WHITE); }

void clear(Color color) {
    beginDraw();
    State& s = st();
    // Fill the first row, then copy it to the others.
    const std::size_t rowBytes = static_cast<std::size_t>(s.pw) * 4;
    for (std::size_t i = 0; i < rowBytes; i += 4) {
        s.pixels[i] = color.r;
        s.pixels[i + 1] = color.g;
        s.pixels[i + 2] = color.b;
        s.pixels[i + 3] = color.a;
    }
    for (std::size_t off = rowBytes; off < s.pixels.size(); off += rowBytes) {
        std::memcpy(&s.pixels[off], s.pixels.data(), rowBytes);
    }
    afterDraw();
}

void enableDoubleBuffering() {
    State& s = st();
    s.doubleBuffered = true;
    if (s.renderer) SDL_SetRenderVSync(s.renderer, 1);
}

void disableDoubleBuffering() {
    State& s = st();
    s.doubleBuffered = false;
    if (s.renderer) SDL_SetRenderVSync(s.renderer, 0);
    if (s.initialized) present();
}

void show() {
    endFrame();
    present();
    pollEvents();
}

void pause(int ms) {
    if (ms < 0) fail("pause: ms must not be negative");
    endFrame();
    State& s = st();
    if (s.headless) return;
    if (!s.doubleBuffered && s.dirty) present();

    // When pause() is called repeatedly, wait relative to the end of the
    // previous pause so the time spent drawing counts toward this one. After
    // a long gap, wait the full time from now.
    const Uint64 length = static_cast<Uint64>(ms) * 1'000'000;
    const Uint64 start = SDL_GetTicksNS();
    Uint64 target = start + length;
    if (s.hasPaused && start - s.pauseEnd < 2 * length) target = s.pauseEnd + length;

    Uint64 now = start;
    while (true) {
        pollEvents();
        now = SDL_GetTicksNS();
        if (now >= target) break;
        SDL_DelayPrecise(std::min<Uint64>(target - now, 10'000'000));
    }
    s.hasPaused = true;
    s.pauseEnd = target > start ? target : now;
}

void save(const std::string& filename) {
    ensureInit();
    draw_internal::writeImageFile(canvasImage(), filename, "draw", "save");
}

image::Image canvas() {
    ensureInit();
    return canvasImage();
}

// --- Mouse -------------------------------------------------------------------

double mouseX() {
    beginInput();
    const State& s = st();
    return s.xmin + s.mouseWX / s.windowW * (s.xmax - s.xmin);
}

double mouseY() {
    beginInput();
    const State& s = st();
    return s.ymax - s.mouseWY / s.windowH * (s.ymax - s.ymin);
}

bool isMousePressed() {
    beginInput();
    if (st().headless) return false;
    return SDL_GetMouseState(nullptr, nullptr) != 0;
}

bool mouseClicked() {
    beginInput();
    State& s = st();
    bool clicked = s.clicked;
    s.clicked = false;
    return clicked;
}

// --- Keyboard ----------------------------------------------------------------

bool hasNextKeyTyped() {
    beginInput();
    const State& s = st();
    return s.keyRead < s.keyCount;
}

char nextKeyTyped() {
    beginInput();
    State& s = st();
    if (s.keyRead >= s.keyCount) fail("nextKeyTyped: no key was typed (check hasNextKeyTyped() first)");
    return s.keys[static_cast<std::size_t>(s.keyRead++)];
}

bool isKeyPressed(Key key) {
    beginInput();
    if (st().headless) return false;
    const bool* state = SDL_GetKeyboardState(nullptr);
    auto down = [&](SDL_Keycode k) { return state[SDL_GetScancodeFromKey(k, nullptr)]; };
    switch (key) {
    case Key::Shift: return down(SDLK_LSHIFT) || down(SDLK_RSHIFT);
    case Key::Control: return down(SDLK_LCTRL) || down(SDLK_RCTRL);
    case Key::Alt: return down(SDLK_LALT) || down(SDLK_RALT);
    case Key::Enter: return down(SDLK_RETURN) || down(SDLK_KP_ENTER);
    default: return down(keycodeOf(key));
    }
}

}  // namespace draw

void draw_internal::keepWindowAlive() {
    if (draw::st().window) draw::pumpIfDue(SDL_GetTicksNS());
}
