// Tests for image.hpp and the canvas <-> image bridge (canvas::picture with an
// Image, canvas::canvas). Runs headless.

#include <canvas.hpp>
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
    image::setPixel(img, 0, 0, image::RED);    // top left
    image::setPixel(img, 0, 4, image::GREEN);  // top right
    image::setPixel(img, 2, 0, image::BLUE);   // bottom left
    image::setPixel(img, 2, 4, image::rgb(10, 20, 30, 128));
    return img;
}

void testCreate() {
    image::Image img = image::create(4, 3);
    CHECK(img.width == 4 && img.height == 3 && img.pixels.size() == 12);
    CHECK(image::getPixel(img, 2, 3) == image::WHITE);
    image::Image filled = image::create(2, 2, image::BOOK_RED);
    CHECK(image::getPixel(filled, 1, 1) == image::BOOK_RED);
    CHECK(image::create(0, 0).pixels.empty());
}

void testGetSet() {
    image::Image img = corners();
    CHECK(image::getPixel(img, 0, 0) == image::RED);
    CHECK(image::getPixel(img, 0, 4) == image::GREEN);
    CHECK(image::getPixel(img, 2, 0) == image::BLUE);
    CHECK(image::getPixel(img, 1, 2) == image::WHITE);
    CHECK(img.pixels[0] == image::RED);                    // row 0 is the top row
    CHECK(img.pixels[2 * 5 + 0] == image::BLUE);           // pixels[row * width + col]
}

void testIndexOperator() {
    image::Image img = corners();  // 5 wide, 3 high: (row, col) and (col, row) differ
    CHECK(img[0][0] == image::RED);
    CHECK(img[0][4] == image::GREEN);  // row 0, col 4: top right
    CHECK(img[2][0] == image::BLUE);   // row 2, col 0: bottom left
    img[1][3] = image::BOOK_BLUE;
    CHECK(image::getPixel(img, 1, 3) == image::BOOK_BLUE);  // the same order everywhere
    CHECK(img.pixels[1 * 5 + 3] == image::BOOK_BLUE);
    img[1][3].g = 0;  // components can be changed directly
    CHECK(image::getPixel(img, 1, 3).g == 0);

    const image::Image& view = img;  // read-only access works too
    CHECK(view[2][4] == image::rgb(10, 20, 30, 128));

    for (int row = 0; row < img.height; ++row) {  // the usual nested loop
        for (int col = 0; col < img.width; ++col) img[row][col] = image::rgb(row, col, 0);
    }
    CHECK(img[2][4] == image::rgb(2, 4, 0));
}

void testCopiesAreIndependent() {
    image::Image a = corners();
    image::Image b = a;
    image::setPixel(b, 0, 0, image::BLACK);
    CHECK(image::getPixel(a, 0, 0) == image::RED);
    CHECK(image::getPixel(b, 0, 0) == image::BLACK);
}

void testSameColorType() {
    canvas::Color c = image::RED;  // the same type under both names
    image::Color d = canvas::rgb(255, 0, 0);
    CHECK(c == d);
    CHECK(canvas::BOOK_BLUE == image::BOOK_BLUE);
}

void testSaveAndLoad() {
    image::Image img = corners();
    image::save(img, "image_test.png");
    CHECK(image::load("image_test.png").pixels == img.pixels);  // PNG is lossless, keeps alpha

    image::save(img, "image_test.bmp");
    image::Image bmp = image::load("image_test.bmp");
    CHECK(bmp.width == 5 && bmp.height == 3);
    CHECK(image::getPixel(bmp, 0, 0) == image::RED);
    CHECK(image::getPixel(bmp, 2, 0) == image::BLUE);

    image::Image big = image::create(64, 64, image::BOOK_BLUE);  // JPEG is lossy
    image::save(big, "image_test.jpg");
    image::Image jpg = image::load("image_test.jpg");
    CHECK(jpg.width == 64 && near(image::getPixel(jpg, 32, 32), image::BOOK_BLUE, 4));
}

void testCanvas() {
    canvas::setCanvasSize(40, 30);
    image::Image c = canvas::snapshot();
    CHECK(c.width == 40 && c.height == 30);
    CHECK(image::getPixel(c, 15, 20) == canvas::WHITE);

    // The top-left quarter of the canvas, in canvas coordinates (y up).
    canvas::setPenColor(canvas::RED);
    canvas::filledRectangle(0.25, 0.75, 0.25, 0.25);
    c = canvas::snapshot();
    CHECK(image::getPixel(c, 0, 0) == canvas::RED);      // row 0 is the top
    CHECK(image::getPixel(c, 14, 19) == canvas::RED);
    CHECK(image::getPixel(c, 15, 20) == canvas::WHITE);
    CHECK(image::getPixel(c, 29, 39) == canvas::WHITE);
}

void testPictureAtNaturalSizeIsExact() {
    // An image drawn at natural size on a canvas of the same size reproduces
    // it pixel for pixel.
    image::Image img = image::create(40, 30);
    for (int row = 0; row < img.height; ++row) {
        for (int col = 0; col < img.width; ++col) {
            image::setPixel(img, row, col, image::rgb(col * 6, row * 8, (col + row) * 3));
        }
    }
    canvas::setCanvasSize(40, 30);
    canvas::picture(0.5, 0.5, img);
    CHECK(canvas::snapshot().pixels == img.pixels);

    // Round trip: canvas -> image -> picture.
    canvas::clear(canvas::BLACK);
    canvas::picture(0.5, 0.5, img);
    image::Image again = canvas::snapshot();
    canvas::clear();
    canvas::picture(0.5, 0.5, again);
    CHECK(canvas::snapshot().pixels == img.pixels);
}

void testSpeed() {
    // A typical exercise: grayscale a 1000 x 1000 image with get() and set().
    image::Image src = image::create(1000, 1000, image::BOOK_LIGHT_BLUE);
    image::Image out = image::create(src.width, src.height);
    auto start = std::chrono::steady_clock::now();
    for (int row = 0; row < src.height; ++row) {
        for (int col = 0; col < src.width; ++col) {
            image::Color c = image::getPixel(src, row, col);
            int gray = (299 * c.r + 587 * c.g + 114 * c.b) / 1000;
            image::setPixel(out, row, col, image::rgb(gray, gray, gray));
        }
    }
    double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    std::printf("grayscale of 1000 x 1000 with getPixel/setPixel: %.1f ms\n", ms);

    start = std::chrono::steady_clock::now();
    for (int row = 0; row < src.height; ++row) {
        for (int col = 0; col < src.width; ++col) {
            image::Color c = src[row][col];
            int gray = (299 * c.r + 587 * c.g + 114 * c.b) / 1000;
            out[row][col] = image::rgb(gray, gray, gray);
        }
    }
    ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    std::printf("grayscale of 1000 x 1000 with [row][col]: %.1f ms\n", ms);
    CHECK(image::getPixel(out, 500, 500).r == image::getPixel(out, 500, 500).b);
}

}  // namespace

int main() {
    testCreate();
    testGetSet();
    testIndexOperator();
    testCopiesAreIndependent();
    testSameColorType();
    testSaveAndLoad();
    testCanvas();
    testPictureAtNaturalSizeIsExact();
    testSpeed();
    return finish();
}
