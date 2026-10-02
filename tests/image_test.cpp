// Tests for image.hpp and the draw <-> image bridge (draw::picture with an
// Image, draw::canvas). Runs headless.

#include <draw.hpp>
#include <image.hpp>

#include <chrono>
#include <cstdio>
#include <cstdlib>

#include "check.hpp"

namespace {

bool near(image::Color x, image::Color y, int tolerance) {
    return std::abs(x.r - y.r) <= tolerance && std::abs(x.g - y.g) <= tolerance &&
           std::abs(x.b - y.b) <= tolerance && std::abs(x.a - y.a) <= tolerance;
}

// A small image with a different color in each corner.
image::Image corners() {
    image::Image img = image::create(5, 3, image::WHITE);
    image::set(img, 0, 0, image::RED);    // top left
    image::set(img, 4, 0, image::GREEN);  // top right
    image::set(img, 0, 2, image::BLUE);   // bottom left
    image::set(img, 4, 2, image::rgb(10, 20, 30, 128));
    return img;
}

void testCreate() {
    image::Image img = image::create(4, 3);
    CHECK(img.width == 4 && img.height == 3 && img.pixels.size() == 12);
    CHECK(image::get(img, 3, 2) == image::WHITE);
    image::Image filled = image::create(2, 2, image::BOOK_RED);
    CHECK(image::get(filled, 1, 1) == image::BOOK_RED);
    CHECK(image::create(0, 0).pixels.empty());
}

void testGetSet() {
    image::Image img = corners();
    CHECK(image::get(img, 0, 0) == image::RED);
    CHECK(image::get(img, 4, 0) == image::GREEN);
    CHECK(image::get(img, 0, 2) == image::BLUE);
    CHECK(image::get(img, 2, 1) == image::WHITE);
    CHECK(img.pixels[0] == image::RED);                    // row 0 is the top row
    CHECK(img.pixels[2 * 5 + 0] == image::BLUE);           // pixels[row * width + col]
}

void testCopiesAreIndependent() {
    image::Image a = corners();
    image::Image b = a;
    image::set(b, 0, 0, image::BLACK);
    CHECK(image::get(a, 0, 0) == image::RED);
    CHECK(image::get(b, 0, 0) == image::BLACK);
}

void testSameColorType() {
    draw::Color c = image::RED;  // the same type under both names
    image::Color d = draw::rgb(255, 0, 0);
    CHECK(c == d);
    CHECK(draw::BOOK_BLUE == image::BOOK_BLUE);
}

void testSaveAndLoad() {
    image::Image img = corners();
    image::save(img, "image_test.png");
    CHECK(image::load("image_test.png").pixels == img.pixels);  // PNG is lossless, keeps alpha

    image::save(img, "image_test.bmp");
    image::Image bmp = image::load("image_test.bmp");
    CHECK(bmp.width == 5 && bmp.height == 3);
    CHECK(image::get(bmp, 0, 0) == image::RED);
    CHECK(image::get(bmp, 0, 2) == image::BLUE);

    image::Image big = image::create(64, 64, image::BOOK_BLUE);  // JPEG is lossy
    image::save(big, "image_test.jpg");
    image::Image jpg = image::load("image_test.jpg");
    CHECK(jpg.width == 64 && near(image::get(jpg, 32, 32), image::BOOK_BLUE, 4));
}

void testCanvas() {
    draw::setCanvasSize(40, 30);
    image::Image c = draw::canvas();
    CHECK(c.width == 40 && c.height == 30);
    CHECK(image::get(c, 20, 15) == draw::WHITE);

    // The top-left quarter of the canvas, in draw coordinates (y up).
    draw::setPenColor(draw::RED);
    draw::filledRectangle(0.25, 0.75, 0.25, 0.25);
    c = draw::canvas();
    CHECK(image::get(c, 0, 0) == draw::RED);      // row 0 is the top
    CHECK(image::get(c, 19, 14) == draw::RED);
    CHECK(image::get(c, 20, 15) == draw::WHITE);
    CHECK(image::get(c, 39, 29) == draw::WHITE);
}

void testPictureAtNaturalSizeIsExact() {
    // An image drawn at natural size on a canvas of the same size reproduces
    // it pixel for pixel.
    image::Image img = image::create(40, 30);
    for (int row = 0; row < img.height; ++row) {
        for (int col = 0; col < img.width; ++col) {
            image::set(img, col, row, image::rgb(col * 6, row * 8, (col + row) * 3));
        }
    }
    draw::setCanvasSize(40, 30);
    draw::picture(0.5, 0.5, img);
    CHECK(draw::canvas().pixels == img.pixels);

    // Round trip: canvas -> image -> picture.
    draw::clear(draw::BLACK);
    draw::picture(0.5, 0.5, img);
    image::Image again = draw::canvas();
    draw::clear();
    draw::picture(0.5, 0.5, again);
    CHECK(draw::canvas().pixels == img.pixels);
}

void testSpeed() {
    // A typical exercise: grayscale a 1000 x 1000 image with get() and set().
    image::Image src = image::create(1000, 1000, image::BOOK_LIGHT_BLUE);
    image::Image out = image::create(src.width, src.height);
    auto start = std::chrono::steady_clock::now();
    for (int row = 0; row < src.height; ++row) {
        for (int col = 0; col < src.width; ++col) {
            image::Color c = image::get(src, col, row);
            int gray = (299 * c.r + 587 * c.g + 114 * c.b) / 1000;
            image::set(out, col, row, image::rgb(gray, gray, gray));
        }
    }
    double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    std::printf("grayscale of 1000 x 1000 with get/set: %.1f ms\n", ms);
    CHECK(image::get(out, 500, 500).r == image::get(out, 500, 500).b);
}

}  // namespace

int main() {
    testCreate();
    testGetSet();
    testCopiesAreIndependent();
    testSameColorType();
    testSaveAndLoad();
    testCanvas();
    testPictureAtNaturalSizeIsExact();
    testSpeed();
    return finish();
}
