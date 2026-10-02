// Draws a small scene, copies it from the canvas into an image, and shows a
// grayscale and a mirrored version next to it. Pass an image file name to
// use that picture instead: ./image_effects photo.png

#include <canvas.hpp>
#include <image.hpp>

image::Image grayscale(const image::Image& src) {
    image::Image out = image::create(src.width, src.height);
    for (int row = 0; row < src.height; ++row) {
        for (int col = 0; col < src.width; ++col) {
            image::Color c = image::get(src, col, row);
            int gray = (299 * c.r + 587 * c.g + 114 * c.b) / 1000;
            image::set(out, col, row, image::rgb(gray, gray, gray));
        }
    }
    return out;
}

image::Image mirror(const image::Image& src) {
    image::Image out = image::create(src.width, src.height);
    for (int row = 0; row < src.height; ++row) {
        for (int col = 0; col < src.width; ++col) {
            image::set(out, src.width - 1 - col, row, image::get(src, col, row));
        }
    }
    return out;
}

image::Image scene() {
    canvas::setCanvasSize(200, 200);
    canvas::clear(canvas::BOOK_LIGHT_BLUE);
    canvas::setPenColor(canvas::ORANGE);
    canvas::filledCircle(0.25, 0.75, 0.15);
    canvas::setPenColor(canvas::rgb(40, 140, 60));
    canvas::filledRectangle(0.5, 0.15, 0.5, 0.15);
    canvas::setPenColor(canvas::BOOK_RED);
    canvas::filledPolygon({{0.55, 0.3}, {0.9, 0.3}, {0.725, 0.6}});
    return canvas::snapshot();
}

int main(int argc, char** argv) {
    image::Image original = argc > 1 ? image::load(argv[1]) : scene();

    canvas::setTitle("Image effects");
    canvas::setCanvasSize(3 * original.width, original.height);
    canvas::setXscale(0, 3);
    canvas::picture(0.5, 0.5, original);
    canvas::picture(1.5, 0.5, grayscale(original));
    canvas::picture(2.5, 0.5, mirror(original));
}
