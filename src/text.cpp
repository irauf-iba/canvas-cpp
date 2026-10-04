// text.cpp - fonts and text: the built-in font, setFont(), and drawing text
// with stb_truetype.

#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "canvas_impl.hpp"

// stb_truetype, compiled into this file with internal linkage so it cannot
// clash with a copy of stb in the student's own program.
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace canvas::impl {

namespace {

const unsigned char kBuiltinFont[] = {
#include "font_data.inc"
};

struct Glyph {
    int w = 0, h = 0, xoff = 0, yoff = 0;
    std::vector<std::uint8_t> alpha;
};

struct Font {
    std::vector<unsigned char> file;  // empty for the built-in font
    stbtt_fontinfo info{};
    std::map<std::tuple<int, int, int>, Glyph> glyphs;  // (glyph, size * 64, subpixel x)
    bool ready = false;
};

// The current font, built in or from setFont(). Never freed, like the state.
Font& theFont() {
    static Font* font = new Font;
    return *font;
}

}  // namespace

// ---------------------------------------------------------------------------
// Text
// ---------------------------------------------------------------------------

Font& currentFont() {
    Font& font = theFont();
    if (!font.ready) {
        stbtt_InitFont(&font.info, kBuiltinFont, stbtt_GetFontOffsetForIndex(kBuiltinFont, 0));
        font.ready = true;
    }
    return font;
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

// Draws text anchored at pixel (px, py); see drawText.
void drawTextPX(double px, double py, const std::string& text, double align, double degrees) {
    TextMask m = renderText(text, align);
    if (m.w == 0 || m.h == 0) return;
    PenBlender pen;

    if (degrees == 0) {
        // Snap to whole pixels so unrotated text stays sharp.
        int left = static_cast<int>(std::lround(px - m.anchorX));
        int top = static_cast<int>(std::lround(py - m.anchorY));
        Box box = clipBox(left, top, left + m.w, top + m.h);
        if (box.empty()) noteOffscreen(left, top, left + m.w, top + m.h);
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
    if (box.empty()) noteOffscreen(minX - 1, minY - 1, maxX + 1, maxY + 1);
    for (int cy = box.y0; cy < box.y1; ++cy) {
        for (int cx = box.x0; cx < box.x1; ++cx) {
            double dx = cx + 0.5 - px, dy = cy + 0.5 - py;
            double u = dx * c - dy * s, v = dx * s + dy * c;
            double a = sampleMask(m, m.anchorX + u, m.anchorY + v);
            if (a > 1e-4) pen(cx, cy, a);
        }
    }
}

void drawText(double x, double y, const std::string& text, double align, double degrees) {
    drawTextPX(toPX(x), toPY(y), text, align, degrees);
}

}  // namespace canvas::impl

namespace canvas {

using namespace impl;

void setFont(const std::string& ttfFile) {
    const std::string path = canvas_internal::findInputFile(ttfFile);
    if (path.empty()) fail("setFont: " + canvas_internal::notFound(ttfFile));
    std::ifstream in(path, std::ios::binary);
    if (!in) fail("setFont: cannot read '" + ttfFile + "'");
    std::vector<unsigned char> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    Font font;
    font.file = std::move(data);
    int offset = stbtt_GetFontOffsetForIndex(font.file.data(), 0);
    if (offset < 0 || !stbtt_InitFont(&font.info, font.file.data(), offset)) {
        fail("setFont: '" + ttfFile + "' is not a TrueType font");
    }
    // stbtt_fontinfo points into font.file, whose buffer survives the move.
    font.ready = true;
    theFont() = std::move(font);
}

void setFontSize(double pixels) {
    checkFinite("setFontSize", {pixels});
    if (pixels <= 0) fail("setFontSize: the size must be positive");
    st().fontSize = pixels;
}

// --- Text and images ---------------------------------------------------------

std::string detail::numberToText(double value) {
    char buffer[64];
    if (std::isfinite(value) && value == std::floor(value) && std::abs(value) < 1e15) {
        std::snprintf(buffer, sizeof buffer, "%.0f", value);  // 1000000, not 1e+06
    } else {
        std::snprintf(buffer, sizeof buffer, "%g", value);    // 3.14159
    }
    return buffer;
}

void text(double x, double y, const std::string& s) {
    checkFinite("text", {x, y});
    beginDraw("text", x, y);
    drawText(x, y, s, 0.5, 0);
    afterDraw();
}

void textLeft(double x, double y, const std::string& s) {
    checkFinite("textLeft", {x, y});
    beginDraw("textLeft", x, y);
    drawText(x, y, s, 0, 0);
    afterDraw();
}

void textRight(double x, double y, const std::string& s) {
    checkFinite("textRight", {x, y});
    beginDraw("textRight", x, y);
    drawText(x, y, s, 1, 0);
    afterDraw();
}

void text(double x, double y, const std::string& s, double degrees) {
    checkFinite("text", {x, y, degrees});
    beginDraw("text", x, y);
    drawText(x, y, s, 0.5, std::fmod(degrees, 360.0));
    afterDraw();
}

}  // namespace canvas
