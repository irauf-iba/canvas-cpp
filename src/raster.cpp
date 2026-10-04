// raster.cpp - turning shapes and images into pixels.
//
// Rasterization works in physical pixels, where pixel (i, j) covers the square
// [i, i+1] x [j, j+1] and y points down. Filled shapes use exact-area coverage
// accumulation (as in font-rs); strokes use distance to the line segments, which
// gives round joins and caps and overlaps without seams.

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "canvas_impl.hpp"

namespace canvas::impl {

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
    if (box.empty()) {
        noteOffscreen(minX, minY, maxX, maxY);
        return;
    }

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
    if (box.empty()) {
        noteOffscreen(minX - reach, minY - reach, maxX + reach, maxY + reach);
        return;
    }

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
// Images
// ---------------------------------------------------------------------------

const image::Image& loadPicture(const std::string& filename) {
    State& s = st();
    auto it = s.pictures.find(filename);
    if (it != s.pictures.end()) return it->second;
    image::Image img = canvas_internal::readImageFile(filename, "canvas", "picture");
    return s.pictures.emplace(filename, std::move(img)).first->second;
}

// Draws the image centered at pixel (cx, cy), scaled to dw x dh pixels,
// with bilinear filtering on premultiplied colors.
void drawImagePX(const image::Image& img, double cx, double cy, double dw, double dh) {
    if (dw <= 0 || dh <= 0 || img.width == 0 || img.height == 0) return;
    const double left = std::round(cx - dw / 2), top = std::round(cy - dh / 2);
    Box box = clipBox(left, top, left + dw, top + dh);
    if (box.empty()) noteOffscreen(left, top, left + dw, top + dh);
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

// Draws the image scaled to dw x dh pixels and turned counterclockwise by
// degrees around pixel (cx, cy). Each canvas pixel is read from where it
// comes from in the image, between pixels, with transparency outside the
// image so the edges are smooth.
void drawImagePX(const image::Image& img, double cx, double cy, double dw, double dh, double degrees) {
    const double turn = std::fmod(degrees, 360.0);
    if (turn == 0) {
        drawImagePX(img, cx, cy, dw, dh);
        return;
    }
    if (dw <= 0 || dh <= 0 || img.width == 0 || img.height == 0) return;
    const double pi = 3.14159265358979323846;
    const double c = std::cos(turn * pi / 180), s = std::sin(turn * pi / 180);
    const double halfW = (std::abs(dw * c) + std::abs(dh * s)) / 2, halfH = (std::abs(dw * s) + std::abs(dh * c)) / 2;
    Box box = clipBox(cx - halfW, cy - halfH, cx + halfW, cy + halfH);
    if (box.empty()) noteOffscreen(cx - halfW, cy - halfH, cx + halfW, cy + halfH);
    const double sx = img.width / dw, sy = img.height / dh;
    auto texel = [&](int x, int y) {
        return x < 0 || y < 0 || x >= img.width || y >= img.height
                   ? Color{0, 0, 0, 0}
                   : img.pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(img.width) +
                                static_cast<std::size_t>(x)];
    };
    for (int y = box.y0; y < box.y1; ++y) {
        for (int x = box.x0; x < box.x1; ++x) {
            // Turn back by the angle (y points down on the screen).
            const double dx = x + 0.5 - cx, dy = y + 0.5 - cy;
            const double u = (dx * c - dy * s + dw / 2) * sx - 0.5, v = (dx * s + dy * c + dh / 2) * sy - 0.5;
            if (u < -1 || v < -1 || u > img.width || v > img.height) continue;
            const int i = static_cast<int>(std::floor(u)), j = static_cast<int>(std::floor(v));
            const double fu = u - i, fv = v - j;
            const Color q[4] = {texel(i, j), texel(i + 1, j), texel(i, j + 1), texel(i + 1, j + 1)};
            const double wt[4] = {(1 - fu) * (1 - fv), fu * (1 - fv), (1 - fu) * fv, fu * fv};
            double sum[4] = {0, 0, 0, 0};
            for (int n = 0; n < 4; ++n) {
                double a = wt[n] * q[n].a;
                sum[0] += q[n].r * a;
                sum[1] += q[n].g * a;
                sum[2] += q[n].b * a;
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

}  // namespace canvas::impl

