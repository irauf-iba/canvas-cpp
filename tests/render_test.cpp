// Snapshot tests: draws a scene headless, saves it, and compares it with a
// reference image.
//
// Usage: render_test <scene> <reference-dir>
//
// Small differences (e.g. from a different math library) are tolerated. To
// accept new output as the reference, run with CANVAS_UPDATE_REFERENCES=1.
// On failure, <scene>-diff.png marks the differing pixels in red.

#include <canvas.hpp>
#include <stats.hpp>
#include <turtle.hpp>

#include "internal.hpp"  // screenImage(), for the overlay scene

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <map>
#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace {

const double kPi = 3.14159265358979323846;

void shapes() {
    canvas::setCanvasSize(400, 400);
    canvas::setPenColor(canvas::BOOK_BLUE);
    canvas::filledCircle(0.12, 0.8, 0.09);
    canvas::filledSquare(0.37, 0.8, 0.09);
    canvas::filledRectangle(0.62, 0.8, 0.11, 0.06);
    canvas::filledEllipse(0.87, 0.8, 0.11, 0.06);

    canvas::setPenColor(canvas::BOOK_RED);
    canvas::setPenWidth(3);
    canvas::circle(0.12, 0.5, 0.09);
    canvas::square(0.37, 0.5, 0.09);
    canvas::rectangle(0.62, 0.5, 0.11, 0.06);
    canvas::ellipse(0.87, 0.5, 0.11, 0.06);

    canvas::setPenColor(canvas::BLACK);
    for (int i = 0; i < 6; ++i) {  // pen widths 0.5 to 8
        canvas::setPenWidth(0.5 + i * 1.5);
        canvas::line(0.05 + i * 0.07, 0.1, 0.1 + i * 0.07, 0.3);
    }
    canvas::setPenWidth(4);
    canvas::arc(0.62, 0.2, 0.09, 45, 315);
    canvas::arc(0.87, 0.2, 0.09, 300, 60);  // wraps past 360
}

void polygons() {
    canvas::setCanvasSize(400, 400);
    std::vector<canvas::Point> star;
    for (int i = 0; i < 5; ++i) {
        double t = kPi / 2 + i * 4 * kPi / 5;
        star.push_back({0.15 + 0.12 * std::cos(t), 0.7 + 0.12 * std::sin(t)});
    }
    canvas::setPenColor(canvas::rgb(40, 160, 60));
    canvas::filledPolygon(star);  // nonzero rule: the center is filled
    canvas::setPenColor(canvas::BLACK);
    canvas::setPenWidth(1);
    canvas::polygon(star);

    // Translucent overlapping circles.
    canvas::setPenColor(canvas::rgb(255, 0, 0, 128));
    canvas::filledCircle(0.45, 0.72, 0.1);
    canvas::setPenColor(canvas::rgb(0, 0, 255, 128));
    canvas::filledCircle(0.55, 0.65, 0.1);

    // The vector-of-coordinates overloads.
    canvas::setPenColor(canvas::ORANGE);
    canvas::filledPolygon({0.75, 0.95, 0.85}, {0.6, 0.6, 0.85});
    canvas::setPenColor(canvas::BLACK);
    canvas::setPenWidth(2);
    canvas::polygon({0.75, 0.95, 0.85}, {0.6, 0.6, 0.85});

    std::vector<double> xs, ys;
    for (int i = 0; i <= 100; ++i) {
        xs.push_back(0.05 + 0.9 * i / 100);
        ys.push_back(0.3 + 0.1 * std::sin(i * 0.25));
    }
    canvas::setPenColor(canvas::MAGENTA);
    canvas::polyline(xs, ys);

    canvas::setPenColor(canvas::BLACK);
    for (int i = 0; i < 6; ++i) {
        canvas::setPenWidth(1 + i);
        canvas::point(0.1 + i * 0.16, 0.08);
    }
}

void text() {
    canvas::setCanvasSize(400, 300);
    canvas::setPenColor(canvas::LIGHT_GRAY);
    canvas::setPenWidth(1);
    canvas::line(0.5, 0, 0.5, 1);
    for (double y : {0.85, 0.7, 0.55}) canvas::line(0, y, 1, y);

    canvas::setPenColor(canvas::BLACK);
    canvas::text(0.5, 0.85, "Centered: Hello, draw!");
    canvas::textLeft(0.5, 0.7, "textLeft");
    canvas::textRight(0.5, 0.7, "textRight");
    canvas::setFontSize(12);
    canvas::text(0.5, 0.55, "Kerning AVAWAY  Latin-1 \xC3\x84\xC3\xB6\xC3\xBC  Greek \xCE\xA9\xCF\x80");
    canvas::setFontSize(28);
    canvas::setPenColor(canvas::BOOK_BLUE);
    canvas::text(0.25, 0.25, "rotated", 30);
    canvas::text(0.6, 0.25, "90", 90);
    canvas::text(0.85, 0.25, "-45", -45);
}

void pictures() {
    // Make a 100 x 100 sprite with a transparent background, then draw it back.
    canvas::setCanvasSize(100, 100);
    canvas::clear(canvas::rgb(0, 0, 0, 0));
    canvas::setPenColor(canvas::RED);
    canvas::filledCircle(0.5, 0.5, 0.45);
    canvas::setPenColor(canvas::WHITE);
    canvas::filledSquare(0.5, 0.5, 0.15);
    canvas::save("pictures-sprite.png");

    canvas::setCanvasSize(400, 300);
    canvas::clear(canvas::LIGHT_GRAY);
    canvas::picture(0.15, 0.7, "pictures-sprite.png");                // natural size
    canvas::picture(0.45, 0.7, "pictures-sprite.png", 0.2, 0.1);      // squashed
    canvas::picture(0.8, 0.6, "pictures-sprite.png", 0.4, 0.6);       // enlarged
    canvas::picture(0.15, 0.2, "pictures-sprite.png", 0.05, 0.05);    // shrunk
    canvas::picture(1.0, 0.0, "pictures-sprite.png");                 // partly off canvas
}

void clipping() {
    canvas::setCanvasSize(300, 300);
    canvas::setScale(-1, 1);
    canvas::setPenColor(canvas::ORANGE);
    canvas::filledCircle(1, -1, 0.4);        // corner
    canvas::filledCircle(-1.2, 0, 0.3);      // mostly outside
    canvas::filledPolygon({{-50, -50}, {0, -0.5}, {-50, 50}});  // far outside
    canvas::setPenColor(canvas::BLACK);
    canvas::setPenWidth(3);
    canvas::line(-1e12, 0.5, 1e12, 0.6);     // huge coordinates
    canvas::circle(0, 0, 5);                 // larger than the canvas

    canvas::setXscale(1, -1);                // flipped x axis
    canvas::setPenColor(canvas::BOOK_BLUE);
    canvas::filledSquare(0.6, 0.6, 0.2);     // appears on the left
}

void aspect() {
    // A rectangular canvas: circles, squares and arcs stay round and square,
    // while ellipses and rectangles follow the separate x and y scales.
    canvas::setCanvasSize(400, 200);
    canvas::setPenColor(canvas::BOOK_BLUE);
    canvas::filledCircle(0.1, 0.75, 0.07);
    canvas::filledSquare(0.3, 0.75, 0.07);
    canvas::setPenColor(canvas::BOOK_RED);
    canvas::setPenWidth(3);
    canvas::circle(0.5, 0.75, 0.07);
    canvas::square(0.7, 0.75, 0.07);
    canvas::arc(0.9, 0.75, 0.07, 0, 270);

    // A plot with very different x and y scales: the markers stay round.
    canvas::setXscale(0, 100);
    canvas::setYscale(-3, 1);
    std::vector<canvas::Point> curve;
    for (int i = 0; i <= 100; ++i) curve.push_back({static_cast<double>(i), 0.8 * std::sin(i * 0.12) - 1.5});
    canvas::setPenColor(canvas::GRAY);
    canvas::setPenWidth(1);
    canvas::polyline(curve);
    canvas::setPenColor(canvas::BLACK);
    for (int i = 0; i <= 100; i += 10) canvas::filledCircle(i, curve[static_cast<std::size_t>(i)].y, 1.2);
    canvas::setPenColor(canvas::ORANGE);
    canvas::filledEllipse(50, -2.6, 20, 0.25);  // follows the scales on purpose
}

void imageScene() {
    // An image built pixel by pixel: a gradient with a red marker in the top
    // left corner (row 0 is the top).
    image::Image src = image::create(100, 100);
    for (int row = 0; row < 100; ++row) {
        for (int col = 0; col < 100; ++col) {
            image::Color c = image::rgb(col * 255 / 99, row * 255 / 99, 160);
            if (col < 25 && row < 25) c = image::RED;
            image::setPixel(src, row, col, c);
        }
    }
    // Grayscale and mirrored copies.
    image::Image gray = image::create(100, 100);
    image::Image mirror = image::create(100, 100);
    for (int row = 0; row < 100; ++row) {
        for (int col = 0; col < 100; ++col) {
            image::Color c = image::getPixel(src, row, col);
            int y = (299 * c.r + 587 * c.g + 114 * c.b) / 1000;
            gray[row][col] = image::rgb(y, y, y);
            mirror[row][99 - col] = c;
        }
    }
    canvas::setCanvasSize(400, 200);
    canvas::clear(canvas::LIGHT_GRAY);
    canvas::picture(0.125, 0.7, src);
    canvas::picture(0.375, 0.7, gray);
    canvas::picture(0.625, 0.7, mirror);
    canvas::picture(0.875, 0.5, src, 0.2, 0.9);  // scaled

    // Canvas round trip: grab the canvas, invert the colors of the bottom
    // left area in the image, and draw it back.
    canvas::setPenColor(canvas::BOOK_BLUE);
    canvas::filledCircle(0.2, 0.15, 0.1);
    image::Image c = canvas::snapshot();
    for (int row = 140; row < 200; ++row) {
        for (int col = 0; col < 150; ++col) {
            image::Color p = image::getPixel(c, row, col);
            image::setPixel(c, row, col, image::rgb(255 - p.r, 255 - p.g, 255 - p.b));
        }
    }
    canvas::picture(0.5, 0.5, c);
}

void koch(int n, double size) {
    if (n == 0) {
        turtle::forward(size);
        return;
    }
    koch(n - 1, size / 3);
    turtle::turnLeft(60);
    koch(n - 1, size / 3);
    turtle::turnRight(120);
    koch(n - 1, size / 3);
    turtle::turnLeft(60);
    koch(n - 1, size / 3);
}

void turtleScene() {
    // A Koch snowflake, a square spiral and a dashed line, drawn by the
    // turtle with the canvas pen.
    canvas::setCanvasSize(400, 400);
    canvas::setScale(-1, 1);
    canvas::setPenColor(canvas::BOOK_BLUE);
    canvas::setPenWidth(2);
    turtle::penUp();
    turtle::moveTo(-0.9, 0.35);
    turtle::penDown();
    for (int side = 0; side < 3; ++side) {
        koch(3, 0.9);
        turtle::turnRight(120);
    }

    canvas::setPenColor(canvas::BOOK_RED);
    turtle::penUp();
    turtle::moveTo(0.5, -0.5);
    turtle::setHeading(0);
    turtle::penDown();
    for (int i = 1; i <= 24; ++i) {
        turtle::forward(0.02 * i);
        turtle::turnLeft(90);
    }

    canvas::setPenColor(canvas::BLACK);
    turtle::penUp();
    turtle::moveTo(-0.9, -0.9);
    turtle::setHeading(0);
    for (int i = 0; i < 10; ++i) {
        turtle::penDown();
        turtle::forward(0.06);
        turtle::penUp();
        turtle::forward(0.04);
    }
}

void overlayScene() {
    // What the window shows with the grid and watched values: an automatic
    // grid step for a 0..4 by 0..3 scale, and a few values.
    canvas::setCanvasSize(400, 300);
    canvas::setXscale(0, 4);
    canvas::setYscale(0, 3);
    canvas::setPenColor(canvas::BOOK_RED);
    canvas::filledCircle(2.5, 1.5, 0.6);
    canvas::showGrid();
    canvas::watch("vx", 0.015);
    canvas::watch("score", 120);
    canvas::watch("state", "jumping");
}

void statsScene() {
    // The three plots of one set of values; with a 0 among them, all three
    // fit the same scale.
    const std::vector<double> values = {0, 3, 5, 2, 8, 6, 4, 7, 1, 5};
    canvas::setCanvasSize(400, 300);
    canvas::setPenColor(canvas::BOOK_LIGHT_BLUE);
    stats::plotBars(values);
    canvas::setPenColor(canvas::BOOK_RED);
    canvas::setPenWidth(2);
    stats::plotLines(values);
    canvas::setPenColor(canvas::BLACK);
    stats::plotPoints(values);
}

// --- comparison --------------------------------------------------------------

struct Pixels {
    int w = 0, h = 0;
    std::vector<unsigned char> rgba;
};

bool load(const std::string& path, Pixels& out) {
    int channels;
    unsigned char* data = stbi_load(path.c_str(), &out.w, &out.h, &channels, 4);
    if (!data) return false;
    out.rgba.assign(data, data + static_cast<std::size_t>(out.w) * static_cast<std::size_t>(out.h) * 4);
    stbi_image_free(data);
    return true;
}

bool copyFile(const std::string& from, const std::string& to) {
    std::ifstream in(from, std::ios::binary);
    std::ofstream out(to, std::ios::binary);
    out << in.rdbuf();
    return static_cast<bool>(in) && static_cast<bool>(out);
}

int compare(const std::string& scene, const std::string& actualPath, const std::string& referencePath) {
    Pixels actual, reference;
    if (!load(actualPath, actual)) {
        std::printf("cannot read %s\n", actualPath.c_str());
        return 1;
    }
    if (!load(referencePath, reference)) {
        std::printf("cannot read reference %s (run with CANVAS_UPDATE_REFERENCES=1 to create it)\n",
                    referencePath.c_str());
        return 1;
    }
    if (actual.w != reference.w || actual.h != reference.h) {
        std::printf("size %dx%d differs from reference %dx%d\n", actual.w, actual.h, reference.w,
                    reference.h);
        return 1;
    }

    // Tolerances: a few levels of difference anywhere (rounding), and larger
    // differences on at most 0.2% of pixels (e.g. an edge shifted slightly).
    const int kSmall = 4, kLarge = 64;
    const double kMaxChangedFraction = 0.002;
    int maxDiff = 0;
    std::size_t changed = 0, pixels = actual.rgba.size() / 4;
    std::vector<unsigned char> diff(actual.rgba.size());
    for (std::size_t i = 0; i < pixels; ++i) {
        int d = 0;
        for (std::size_t c = 0; c < 4; ++c) {
            d = std::max(d, std::abs(actual.rgba[i * 4 + c] - reference.rgba[i * 4 + c]));
        }
        maxDiff = std::max(maxDiff, d);
        if (d > kSmall) ++changed;
        unsigned char gray = static_cast<unsigned char>(reference.rgba[i * 4 + 1] / 4 + 191);
        diff[i * 4 + 0] = d > kSmall ? 255 : gray;
        diff[i * 4 + 1] = d > kSmall ? 0 : gray;
        diff[i * 4 + 2] = d > kSmall ? 0 : gray;
        diff[i * 4 + 3] = 255;
    }
    double fraction = static_cast<double>(changed) / static_cast<double>(pixels);
    std::printf("%s: max channel difference %d, %.3f%% of pixels differ by more than %d\n",
                scene.c_str(), maxDiff, fraction * 100, kSmall);
    if (maxDiff <= kSmall || (maxDiff <= kLarge && fraction <= kMaxChangedFraction)) return 0;

    std::string diffPath = scene + "-diff.png";
    stbi_write_png(diffPath.c_str(), actual.w, actual.h, 4, diff.data(), actual.w * 4);
    std::printf("FAILED: differs from %s (see %s)\n", referencePath.c_str(), diffPath.c_str());
    return 1;
}

}  // namespace

int main(int argc, char** argv) {
    const std::map<std::string, std::function<void()>> scenes = {
        {"shapes", shapes}, {"polygons", polygons}, {"text", text},
        {"pictures", pictures}, {"clipping", clipping}, {"aspect", aspect},
        {"image", imageScene}, {"turtle", turtleScene}, {"overlay", overlayScene},
        {"stats", statsScene},
    };
    if (argc != 3 || scenes.count(argv[1]) == 0) {
        std::printf("usage: render_test <scene> <reference-dir>\nscenes:");
        for (const auto& entry : scenes) std::printf(" %s", entry.first.c_str());
        std::printf("\n");
        return 2;
    }
    const std::string scene = argv[1];
    const std::string actualPath = scene + ".png";
    const std::string referencePath = std::string(argv[2]) + "/" + scene + ".png";

    scenes.at(scene)();
    if (scene == "overlay") {
        image::save(canvas_internal::screenImage(), actualPath);  // the overlay isn't in the canvas
    } else {
        canvas::save(actualPath);
    }

    const char* update = std::getenv("CANVAS_UPDATE_REFERENCES");
    if (update && std::strcmp(update, "1") == 0) {
        if (!copyFile(actualPath, referencePath)) {
            std::printf("cannot write %s\n", referencePath.c_str());
            return 1;
        }
        std::printf("updated %s\n", referencePath.c_str());
        return 0;
    }
    return compare(scene, actualPath, referencePath);
}
