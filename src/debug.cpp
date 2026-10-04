// debug.cpp - the debugging aids: hints, the grid and watched values (drawn
// over the canvas when it is shown, never into it), and their switches.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "canvas_impl.hpp"

namespace canvas::impl {

// --- Hints ---------------------------------------------------------------------

void hint(unsigned kind, const std::string& message) {
    State& s = st();
    if (!s.hints || (s.hintsShown & kind)) return;
    s.hintsShown |= kind;
    std::fprintf(stderr, "canvas: hint: %s\n", message.c_str());
}

std::string num(double v) {
    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "%g", v);
    return buffer;
}

std::string colourName(Color c) {
    static const std::pair<Color, const char*> names[] = {
        {BLACK, "BLACK"}, {WHITE, "WHITE"}, {GRAY, "GRAY"}, {LIGHT_GRAY, "LIGHT_GRAY"},
        {DARK_GRAY, "DARK_GRAY"}, {RED, "RED"}, {GREEN, "GREEN"}, {BLUE, "BLUE"}, {CYAN, "CYAN"},
        {MAGENTA, "MAGENTA"}, {YELLOW, "YELLOW"}, {ORANGE, "ORANGE"}, {PINK, "PINK"},
        {BROWN, "BROWN"}, {PURPLE, "PURPLE"}, {BOOK_BLUE, "BOOK_BLUE"},
        {BOOK_LIGHT_BLUE, "BOOK_LIGHT_BLUE"}, {BOOK_RED, "BOOK_RED"}};
    for (const auto& n : names) {
        if (n.first == c) return n.second;
    }
    return "rgb(" + std::to_string(c.r) + ", " + std::to_string(c.g) + ", " + std::to_string(c.b) + ")";
}

// Called when a drawing call leaves nothing to draw on the canvas. Gives the
// hint only if its pixel bounding box really lies outside the canvas, not for
// a shape of zero size inside it.
void noteOffscreen(double minX, double minY, double maxX, double maxY) {
    State& s = st();
    s.drewOffscreen = true;
    if (!*s.drawFunction) return;
    if (maxX >= 0 && minX <= s.pw && maxY >= 0 && minY <= s.ph) return;
    // With the default scale the likely cause is pixel coordinates; otherwise
    // the coordinates and the scale don't match.
    const bool defaultScale = s.xmin == 0 && s.xmax == 1 && s.ymin == 0 && s.ymax == 1;
    hint(kHintOffscreen, std::string(s.drawFunction) + " at (" + num(s.drawX) + ", " + num(s.drawY) +
                             ") is outside the visible area (x from " + num(s.xmin) + " to " + num(s.xmax) +
                             ", y from " + num(s.ymin) + " to " + num(s.ymax) + "). " +
                             (defaultScale ? "Coordinates go from 0 to 1 unless you change them with setScale()."
                                           : "Check the coordinates against the scale."));
}

// ---------------------------------------------------------------------------
// The overlay: grid and watched values
// ---------------------------------------------------------------------------

// A round step giving about ten grid lines across range: 0.1 for 1, 0.5 for 2*pi.
double niceStep(double range) {
    double raw = std::abs(range) / 10;
    double power = std::pow(10, std::floor(std::log10(raw)));
    double f = raw / power;
    return (f < 1.5 ? 1 : f < 3.5 ? 2 : f < 7.5 ? 5 : 10) * power;
}

// A grid value as text with just enough decimals for the step: 0.25, 1.5, 10.
std::string gridLabel(double value, double step) {
    int decimals = 0;
    while (decimals < 6 && std::abs(step * std::pow(10, decimals) - std::round(step * std::pow(10, decimals))) > 1e-9) {
        ++decimals;
    }
    if (std::abs(value) < step * 1e-9) value = 0;  // no "-0"
    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "%.*f", decimals, value);
    return buffer;
}

bool hasOverlay() {
    const State& s = st();
    return s.gridXStep > 0 || !s.watches.empty();
}

void drawGrid() {
    State& s = st();
    const double scale = s.scale;
    const double fontSize = s.fontSize;
    s.fontSize = 11;
    for (int axis = 0; axis < 2; ++axis) {
        const bool vertical = axis == 0;  // vertical lines, at x values
        const double lo = vertical ? std::min(s.xmin, s.xmax) : std::min(s.ymin, s.ymax);
        const double hi = vertical ? std::max(s.xmin, s.xmax) : std::max(s.ymin, s.ymax);
        double step = vertical ? s.gridXStep : s.gridYStep;
        while ((hi - lo) / step > 200) step *= 10;  // a readable number of lines
        for (double k = std::ceil(lo / step - 1e-9); k * step <= hi + step * 1e-9; ++k) {
            const double v = k * step;
            const bool zero = std::abs(v) < step * 1e-9;
            s.pen = zero ? rgb(0, 0, 0, 110) : rgb(0, 0, 0, 40);  // axes darker
            std::vector<Vec> line;
            if (vertical) {
                line = {{toPX(v), 0}, {toPX(v), static_cast<double>(s.ph)}};
            } else {
                line = {{0, toPY(v)}, {static_cast<double>(s.pw), toPY(v)}};
            }
            strokePX(line, false, 1 * scale);
            // Labels along the bottom and left edges, except right at the
            // edges, where they would be cut off or run into each other.
            s.pen = rgb(0, 0, 0, 150);
            const double margin = 16 * scale;
            if (vertical) {
                const double px = toPX(v);
                if (px > margin && px < s.pw - margin) drawTextPX(px, s.ph - 8 * scale, gridLabel(v, step), 0.5, 0);
            } else {
                const double py = toPY(v);
                if (py > margin && py < s.ph - margin) drawTextPX(4 * scale, py - 7 * scale, gridLabel(v, step), 0, 0);
            }
        }
    }
    s.fontSize = fontSize;
}

void drawWatches() {
    State& s = st();
    const double scale = s.scale;
    const double fontSize = s.fontSize;
    s.fontSize = 14;
    std::vector<std::string> lines;
    double width = 0;
    for (const auto& w : s.watches) {
        lines.push_back(w.first + " = " + w.second);
        width = std::max(width, static_cast<double>(renderText(lines.back(), 0).w));
    }
    // In the top-right corner, clear of the grid labels along the left and bottom.
    const double pad = 6 * scale, lineHeight = 18 * scale;
    const double x1 = s.pw - 6 * scale, y0 = 6 * scale;
    const double x0 = x1 - width - 2 * pad, y1 = y0 + lineHeight * static_cast<double>(lines.size()) + pad;
    s.pen = rgb(255, 255, 255, 215);
    fillPolygonPX({{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}});
    s.pen = rgb(0, 0, 0, 90);
    strokePX({{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}}, true, 1 * scale);
    s.pen = BLACK;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        drawTextPX(x0 + pad, y0 + pad / 2 + lineHeight * (static_cast<double>(i) + 0.5), lines[i], 0, 0);
    }
    s.fontSize = fontSize;
}

// The canvas with the grid and watched values drawn over it. The overlay is
// drawn into a copy: the canvas itself, save() and snapshot() never see it.
const std::vector<std::uint8_t>& withOverlay() {
    State& s = st();
    s.composed = s.pixels;
    std::swap(s.pixels, s.composed);  // draw into the copy
    const Color pen = s.pen;
    const char* function = s.drawFunction;
    const bool checkVisible = s.checkVisible;
    s.drawFunction = "";  // no hints for the overlay
    s.checkVisible = false;
    if (s.gridXStep > 0) drawGrid();
    if (!s.watches.empty()) drawWatches();
    s.pen = pen;
    s.drawFunction = function;
    s.checkVisible = checkVisible;
    std::swap(s.pixels, s.composed);
    return s.composed;
}

// After a change to the overlay: show it soon, as after a drawing call.
void overlayChanged() {
    State& s = st();
    s.dirty = true;
    if (s.headless || s.doubleBuffered) return;
    if (SDL_GetTicksNS() - s.lastPresent >= kPresentInterval) present();
}

// G with debug keys on.
void toggleGrid() {
    if (st().gridXStep > 0) hideGrid();
    else showGrid();
}

}  // namespace canvas::impl

namespace canvas {

using namespace impl;

void enableHints() { st().hints = true; }
void disableHints() { st().hints = false; }

void showMouseCoordinates() {
    st().showCoordinates = true;
    updateTitle();
}

void hideMouseCoordinates() {
    st().showCoordinates = false;
    updateTitle();
}

void setDrawDelay(int ms) {
    checkNonNegative("setDrawDelay", "ms", ms);
    st().drawDelay = ms;
}

void showGrid() {
    beginDraw();
    State& s = st();
    // One step for both directions when the ranges are similar, so the cells
    // are square; separate steps for, say, x from 0 to 100 and y from -1 to 1.
    const double xRange = std::abs(s.xmax - s.xmin), yRange = std::abs(s.ymax - s.ymin);
    if (std::max(xRange, yRange) <= 3 * std::min(xRange, yRange)) {
        s.gridXStep = s.gridYStep = niceStep(std::max(xRange, yRange));
    } else {
        s.gridXStep = niceStep(xRange);
        s.gridYStep = niceStep(yRange);
    }
    overlayChanged();
}

void showGrid(double step) {
    checkFinite("showGrid", {step});
    if (step <= 0) fail("showGrid: the step must be positive");
    beginDraw();
    st().gridXStep = st().gridYStep = step;
    overlayChanged();
}

void hideGrid() {
    State& s = st();
    s.gridXStep = s.gridYStep = 0;
    if (s.initialized) overlayChanged();
}

void watch(const std::string& name, const std::string& value) {
    beginDraw();
    State& s = st();
    auto it = std::find_if(s.watches.begin(), s.watches.end(), [&](const auto& w) { return w.first == name; });
    if (it != s.watches.end()) {
        if (it->second == value) return;  // nothing new to show
        it->second = value;
    } else {
        s.watches.emplace_back(name, value);
    }
    overlayChanged();
}

void unwatch(const std::string& name) {
    State& s = st();
    auto it = std::find_if(s.watches.begin(), s.watches.end(), [&](const auto& w) { return w.first == name; });
    if (it == s.watches.end()) return;
    s.watches.erase(it);
    if (s.initialized) overlayChanged();
}

void enableDebugKeys() {
    ensureInit();
    st().debugKeys = true;
}

void disableDebugKeys() {
    State& s = st();
    s.debugKeys = false;
    s.debugPaused = false;
    s.debugStep = false;
    updateTitle();
}

}  // namespace canvas

image::Image canvas_internal::screenImage() {
    canvas::impl::ensureInit();
    canvas::impl::State& s = canvas::impl::st();
    if (!canvas::impl::hasOverlay()) return canvas::snapshot();
    const std::vector<std::uint8_t> original = s.pixels;
    s.pixels = canvas::impl::withOverlay();
    image::Image shown = canvas::impl::canvasImage();
    s.pixels = original;
    return shown;
}
