// canvas.cpp - the drawing functions of canvas.hpp: the window size and
// scale, the pen, shapes, pictures, clearing and saving. canvas_impl.hpp
// explains how the canvas module is split into files.
//
// Every drawing call is rasterized immediately into an RGBA canvas in memory,
// on the caller's thread (raster.cpp). SDL is used only to show that canvas
// in a window and to read mouse and keyboard input (window.cpp); there is no
// render thread and no list of shapes.

#include <cmath>
#include <string>
#include <vector>

#include "canvas_impl.hpp"

namespace canvas::impl {

void fail(const std::string& message) { canvas_internal::fail("canvas", message); }

void checkFinite(const char* function, std::initializer_list<double> values) {
    for (double v : values) {
        if (!std::isfinite(v)) fail(std::string(function) + ": an argument is NaN or infinite");
    }
}

void checkNonNegative(const char* function, const char* name, double v) {
    if (v < 0) fail(std::string(function) + ": " + name + " must not be negative");
}

}  // namespace canvas::impl

namespace canvas {

using namespace impl;

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
    st().title = title;
    updateTitle();
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

// --- Shapes ------------------------------------------------------------------

void point(double x, double y) {
    checkFinite("point", {x, y});
    beginDraw("point", x, y);
    strokePX({toPixel(x, y)}, false, std::max(1.0, penWidthPX()));
    afterDraw();
}

void line(double x0, double y0, double x1, double y1) {
    checkFinite("line", {x0, y0, x1, y1});
    beginDraw("line", x0, y0);
    strokePX({toPixel(x0, y0), toPixel(x1, y1)}, false, penWidthPX());
    afterDraw();
}

void circle(double x, double y, double radius) {
    checkFinite("circle", {x, y, radius});
    checkNonNegative("circle", "radius", radius);
    beginDraw("circle", x, y);
    strokePX(ellipsePX(toPixel(x, y), lengthPX(radius), lengthPX(radius), 0, 360, true), true,
             penWidthPX());
    afterDraw();
}

void filledCircle(double x, double y, double radius) {
    checkFinite("filledCircle", {x, y, radius});
    checkNonNegative("filledCircle", "radius", radius);
    beginDraw("filledCircle", x, y);
    fillPolygonPX(ellipsePX(toPixel(x, y), lengthPX(radius), lengthPX(radius), 0, 360, true));
    afterDraw();
}

void ellipse(double x, double y, double halfWidth, double halfHeight) {
    checkFinite("ellipse", {x, y, halfWidth, halfHeight});
    checkNonNegative("ellipse", "halfWidth", halfWidth);
    checkNonNegative("ellipse", "halfHeight", halfHeight);
    beginDraw("ellipse", x, y);
    strokePX(ellipsePX(toPixel(x, y), lengthPX(halfWidth), lengthPY(halfHeight), 0, 360, true),
             true, penWidthPX());
    afterDraw();
}

void filledEllipse(double x, double y, double halfWidth, double halfHeight) {
    checkFinite("filledEllipse", {x, y, halfWidth, halfHeight});
    checkNonNegative("filledEllipse", "halfWidth", halfWidth);
    checkNonNegative("filledEllipse", "halfHeight", halfHeight);
    beginDraw("filledEllipse", x, y);
    fillPolygonPX(ellipsePX(toPixel(x, y), lengthPX(halfWidth), lengthPY(halfHeight), 0, 360, true));
    afterDraw();
}

void arc(double x, double y, double radius, double angle1, double angle2) {
    checkFinite("arc", {x, y, radius, angle1, angle2});
    checkNonNegative("arc", "radius", radius);
    while (angle2 < angle1) angle2 += 360;
    beginDraw("arc", x, y);
    strokePX(ellipsePX(toPixel(x, y), lengthPX(radius), lengthPX(radius), angle1, angle2, false),
             false, penWidthPX());
    afterDraw();
}

void square(double x, double y, double halfLength) {
    checkFinite("square", {x, y, halfLength});
    checkNonNegative("square", "halfLength", halfLength);
    beginDraw("square", x, y);
    strokePX(squarePX(x, y, lengthPX(halfLength)), true, penWidthPX());
    afterDraw();
}

void filledSquare(double x, double y, double halfLength) {
    checkFinite("filledSquare", {x, y, halfLength});
    checkNonNegative("filledSquare", "halfLength", halfLength);
    beginDraw("filledSquare", x, y);
    fillPolygonPX(squarePX(x, y, lengthPX(halfLength)));
    afterDraw();
}

void rectangle(double x, double y, double halfWidth, double halfHeight) {
    checkFinite("rectangle", {x, y, halfWidth, halfHeight});
    checkNonNegative("rectangle", "halfWidth", halfWidth);
    checkNonNegative("rectangle", "halfHeight", halfHeight);
    beginDraw("rectangle", x, y);
    strokePX(rectanglePX(x, y, halfWidth, halfHeight), true, penWidthPX());
    afterDraw();
}

void filledRectangle(double x, double y, double halfWidth, double halfHeight) {
    checkFinite("filledRectangle", {x, y, halfWidth, halfHeight});
    checkNonNegative("filledRectangle", "halfWidth", halfWidth);
    checkNonNegative("filledRectangle", "halfHeight", halfHeight);
    beginDraw("filledRectangle", x, y);
    fillPolygonPX(rectanglePX(x, y, halfWidth, halfHeight));
    afterDraw();
}

void polygon(const std::vector<Point>& vertices) {
    checkPoints("polygon", vertices);
    beginDraw("polygon", vertices);
    strokePX(pointsPX(vertices), true, penWidthPX());
    afterDraw();
}

void filledPolygon(const std::vector<Point>& vertices) {
    checkPoints("filledPolygon", vertices);
    beginDraw("filledPolygon", vertices);
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
    beginDraw("polyline", points);
    strokePX(pointsPX(points), false, penWidthPX());
    afterDraw();
}

void polyline(const std::vector<double>& x, const std::vector<double>& y) {
    polyline(zipPoints("polyline", x, y));
}

void picture(double x, double y, const image::Image& img) {
    checkFinite("picture", {x, y});
    canvas_internal::checkImage(img, "canvas", "picture");
    beginDraw("picture", x, y, false);
    drawImagePX(img, toPX(x), toPY(y), img.width * st().scale, img.height * st().scale);
    afterDraw();
}

void picture(double x, double y, const image::Image& img, double width, double height) {
    checkFinite("picture", {x, y, width, height});
    checkNonNegative("picture", "width", width);
    checkNonNegative("picture", "height", height);
    canvas_internal::checkImage(img, "canvas", "picture");
    beginDraw("picture", x, y, false);
    drawImagePX(img, toPX(x), toPY(y), lengthPX(width), lengthPY(height));
    afterDraw();
}

void picture(double x, double y, const std::string& filename) {
    picture(x, y, loadPicture(filename));
}

void picture(double x, double y, const std::string& filename, double width, double height) {
    picture(x, y, loadPicture(filename), width, height);
}

void picture(double x, double y, const image::Image& img, double degrees) {
    checkFinite("picture", {x, y, degrees});
    canvas_internal::checkImage(img, "canvas", "picture");
    beginDraw("picture", x, y, false);
    drawImagePX(img, toPX(x), toPY(y), img.width * st().scale, img.height * st().scale, degrees);
    afterDraw();
}

void picture(double x, double y, const image::Image& img, double width, double height, double degrees) {
    checkFinite("picture", {x, y, width, height, degrees});
    checkNonNegative("picture", "width", width);
    checkNonNegative("picture", "height", height);
    canvas_internal::checkImage(img, "canvas", "picture");
    beginDraw("picture", x, y, false);
    drawImagePX(img, toPX(x), toPY(y), lengthPX(width), lengthPY(height), degrees);
    afterDraw();
}

void picture(double x, double y, const std::string& filename, double degrees) {
    picture(x, y, loadPicture(filename), degrees);
}

void picture(double x, double y, const std::string& filename, double width, double height, double degrees) {
    picture(x, y, loadPicture(filename), width, height, degrees);
}

// --- Clearing, animation and saving ------------------------------------------

void clear() { clear(WHITE); }

void clear(Color color) {
    beginDraw();
    State& s = st();
    s.background = color;
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

void save(const std::string& filename) {
    ensureInit();
    canvas_internal::writeImageFile(canvasImage(), filename, "canvas", "save");
}

image::Image snapshot() {
    ensureInit();
    return canvasImage();
}

// --- Game helpers --------------------------------------------------------------

double distance(double x0, double y0, double x1, double y1) {
    checkFinite("distance", {x0, y0, x1, y1});
    return std::hypot(x1 - x0, y1 - y0);
}

}  // namespace canvas

void canvas_internal::canvasCenter(double& x, double& y) {
    const canvas::impl::State& s = canvas::impl::st();
    x = (s.xmin + s.xmax) / 2;
    y = (s.ymin + s.ymax) / 2;
}
