// Snapshot tests: draws a scene headless, saves it, and compares it with a
// reference image.
//
// Usage: render_test <scene> <reference-dir>
//
// Small differences (e.g. from a different math library) are tolerated. To
// accept new output as the reference, run with DRAW_UPDATE_REFERENCES=1.
// On failure, <scene>-diff.png marks the differing pixels in red.

#include <draw.hpp>

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
    draw::setCanvasSize(400, 400);
    draw::setPenColor(draw::BOOK_BLUE);
    draw::filledCircle(0.12, 0.8, 0.09);
    draw::filledSquare(0.37, 0.8, 0.09);
    draw::filledRectangle(0.62, 0.8, 0.11, 0.06);
    draw::filledEllipse(0.87, 0.8, 0.11, 0.06);

    draw::setPenColor(draw::BOOK_RED);
    draw::setPenWidth(3);
    draw::circle(0.12, 0.5, 0.09);
    draw::square(0.37, 0.5, 0.09);
    draw::rectangle(0.62, 0.5, 0.11, 0.06);
    draw::ellipse(0.87, 0.5, 0.11, 0.06);

    draw::setPenColor(draw::BLACK);
    for (int i = 0; i < 6; ++i) {  // pen widths 0.5 to 8
        draw::setPenWidth(0.5 + i * 1.5);
        draw::line(0.05 + i * 0.07, 0.1, 0.1 + i * 0.07, 0.3);
    }
    draw::setPenWidth(4);
    draw::arc(0.62, 0.2, 0.09, 45, 315);
    draw::arc(0.87, 0.2, 0.09, 300, 60);  // wraps past 360
}

void polygons() {
    draw::setCanvasSize(400, 400);
    std::vector<draw::Point> star;
    for (int i = 0; i < 5; ++i) {
        double t = kPi / 2 + i * 4 * kPi / 5;
        star.push_back({0.15 + 0.12 * std::cos(t), 0.7 + 0.12 * std::sin(t)});
    }
    draw::setPenColor(draw::rgb(40, 160, 60));
    draw::filledPolygon(star);  // nonzero rule: the center is filled
    draw::setPenColor(draw::BLACK);
    draw::setPenWidth(1);
    draw::polygon(star);

    // Translucent overlapping circles.
    draw::setPenColor(draw::rgb(255, 0, 0, 128));
    draw::filledCircle(0.45, 0.72, 0.1);
    draw::setPenColor(draw::rgb(0, 0, 255, 128));
    draw::filledCircle(0.55, 0.65, 0.1);

    // The vector-of-coordinates overloads.
    draw::setPenColor(draw::ORANGE);
    draw::filledPolygon({0.75, 0.95, 0.85}, {0.6, 0.6, 0.85});
    draw::setPenColor(draw::BLACK);
    draw::setPenWidth(2);
    draw::polygon({0.75, 0.95, 0.85}, {0.6, 0.6, 0.85});

    std::vector<double> xs, ys;
    for (int i = 0; i <= 100; ++i) {
        xs.push_back(0.05 + 0.9 * i / 100);
        ys.push_back(0.3 + 0.1 * std::sin(i * 0.25));
    }
    draw::setPenColor(draw::MAGENTA);
    draw::polyline(xs, ys);

    draw::setPenColor(draw::BLACK);
    for (int i = 0; i < 6; ++i) {
        draw::setPenWidth(1 + i);
        draw::point(0.1 + i * 0.16, 0.08);
    }
}

void text() {
    draw::setCanvasSize(400, 300);
    draw::setPenColor(draw::LIGHT_GRAY);
    draw::setPenWidth(1);
    draw::line(0.5, 0, 0.5, 1);
    for (double y : {0.85, 0.7, 0.55}) draw::line(0, y, 1, y);

    draw::setPenColor(draw::BLACK);
    draw::text(0.5, 0.85, "Centered: Hello, draw!");
    draw::textLeft(0.5, 0.7, "textLeft");
    draw::textRight(0.5, 0.7, "textRight");
    draw::setFontSize(12);
    draw::text(0.5, 0.55, "Kerning AVAWAY  Latin-1 \xC3\x84\xC3\xB6\xC3\xBC  Greek \xCE\xA9\xCF\x80");
    draw::setFontSize(28);
    draw::setPenColor(draw::BOOK_BLUE);
    draw::text(0.25, 0.25, "rotated", 30);
    draw::text(0.6, 0.25, "90", 90);
    draw::text(0.85, 0.25, "-45", -45);
}

void pictures() {
    // Make a 100 x 100 sprite with a transparent background, then draw it back.
    draw::setCanvasSize(100, 100);
    draw::clear(draw::rgb(0, 0, 0, 0));
    draw::setPenColor(draw::RED);
    draw::filledCircle(0.5, 0.5, 0.45);
    draw::setPenColor(draw::WHITE);
    draw::filledSquare(0.5, 0.5, 0.15);
    draw::save("pictures-sprite.png");

    draw::setCanvasSize(400, 300);
    draw::clear(draw::LIGHT_GRAY);
    draw::picture(0.15, 0.7, "pictures-sprite.png");                // natural size
    draw::picture(0.45, 0.7, "pictures-sprite.png", 0.2, 0.1);      // squashed
    draw::picture(0.8, 0.6, "pictures-sprite.png", 0.4, 0.6);       // enlarged
    draw::picture(0.15, 0.2, "pictures-sprite.png", 0.05, 0.05);    // shrunk
    draw::picture(1.0, 0.0, "pictures-sprite.png");                 // partly off canvas
}

void clipping() {
    draw::setCanvasSize(300, 300);
    draw::setScale(-1, 1);
    draw::setPenColor(draw::ORANGE);
    draw::filledCircle(1, -1, 0.4);        // corner
    draw::filledCircle(-1.2, 0, 0.3);      // mostly outside
    draw::filledPolygon({{-50, -50}, {0, -0.5}, {-50, 50}});  // far outside
    draw::setPenColor(draw::BLACK);
    draw::setPenWidth(3);
    draw::line(-1e12, 0.5, 1e12, 0.6);     // huge coordinates
    draw::circle(0, 0, 5);                 // larger than the canvas

    draw::setXscale(1, -1);                // flipped x axis
    draw::setPenColor(draw::BOOK_BLUE);
    draw::filledSquare(0.6, 0.6, 0.2);     // appears on the left
}

void aspect() {
    // A rectangular canvas: circles, squares and arcs stay round and square,
    // while ellipses and rectangles follow the separate x and y scales.
    draw::setCanvasSize(400, 200);
    draw::setPenColor(draw::BOOK_BLUE);
    draw::filledCircle(0.1, 0.75, 0.07);
    draw::filledSquare(0.3, 0.75, 0.07);
    draw::setPenColor(draw::BOOK_RED);
    draw::setPenWidth(3);
    draw::circle(0.5, 0.75, 0.07);
    draw::square(0.7, 0.75, 0.07);
    draw::arc(0.9, 0.75, 0.07, 0, 270);

    // A plot with very different x and y scales: the markers stay round.
    draw::setXscale(0, 100);
    draw::setYscale(-3, 1);
    std::vector<draw::Point> curve;
    for (int i = 0; i <= 100; ++i) curve.push_back({static_cast<double>(i), 0.8 * std::sin(i * 0.12) - 1.5});
    draw::setPenColor(draw::GRAY);
    draw::setPenWidth(1);
    draw::polyline(curve);
    draw::setPenColor(draw::BLACK);
    for (int i = 0; i <= 100; i += 10) draw::filledCircle(i, curve[static_cast<std::size_t>(i)].y, 1.2);
    draw::setPenColor(draw::ORANGE);
    draw::filledEllipse(50, -2.6, 20, 0.25);  // follows the scales on purpose
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
        std::printf("cannot read reference %s (run with DRAW_UPDATE_REFERENCES=1 to create it)\n",
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
    draw::save(actualPath);

    const char* update = std::getenv("DRAW_UPDATE_REFERENCES");
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
