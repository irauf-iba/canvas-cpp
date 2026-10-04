// image.hpp - images as grids of pixels, for image-processing exercises.
//
// Inspired by Princeton's Picture class. An Image is a plain struct you can
// copy, pass to functions and return from them; the functions below create,
// load, save, read and change it. No window is needed. To show an image, use
// canvas::picture() from canvas.hpp.
//
//     image::Image src = image::load("photo.png");
//     image::Image out = image::create(src.width, src.height);
//     for (int row = 0; row < src.height; ++row) {
//         for (int col = 0; col < src.width; ++col) {
//             image::Color c = src[row][col];
//             int gray = (299 * c.r + 587 * c.g + 114 * c.b) / 1000;
//             out[row][col] = image::rgb(gray, gray, gray);
//         }
//     }
//     image::save(out, "gray.png");
//
// Pixels are addressed by row and then column, like a 2D array: img[row][col],
// getPixel(img, row, col) and setPixel(img, row, col, color) all use that
// order. Row 0 is at the top and column 0 at the left, as in image files and
// image editors. (This differs from canvas, where y points up, because rows
// and columns are positions in a grid, not coordinates.)
//
// Errors, such as a missing file or a pixel outside the image, print a
// message and stop the program.

#ifndef CANVAS_IMAGE_HPP
#define CANVAS_IMAGE_HPP

#include <cstddef>
#include <string>
#include <vector>

#include "color.hpp"

namespace image {

struct Image;

namespace detail {
// The position of pixel (row, col) in img.pixels. Stops with an error naming
// `function` if the pixel is outside the image.
std::size_t pixelIndex(const Image& img, int row, int col, const char* function);

// What img[row] gives: one row, to be indexed by column.
struct Row {
    Image* img;
    int row;
    Color& operator[](int col) const;
};
struct ConstRow {
    const Image* img;
    int row;
    const Color& operator[](int col) const;
};
}  // namespace detail

// An image of width x height pixels. pixels holds the rows one after
// another, top row first: pixel (row, col) is pixels[row * width + col].
struct Image {
    int width = 0;
    int height = 0;
    std::vector<Color> pixels;

    // img[row][col] is the pixel in that row and column, to read or change,
    // as in a 2D array:
    //     image::Color c = img[row][col];
    //     img[row][col] = image::RED;
    // Stops with an error if (row, col) is outside the image.
    detail::Row operator[](int row) { return {this, row}; }
    detail::ConstRow operator[](int row) const { return {this, row}; }
};

inline Color& detail::Row::operator[](int col) const {
    return img->pixels[pixelIndex(*img, row, col, "[row][col]")];
}

inline const Color& detail::ConstRow::operator[](int col) const {
    return img->pixels[pixelIndex(*img, row, col, "[row][col]")];
}

// A new image filled with the color (default WHITE).
Image create(int width, int height);
Image create(int width, int height, Color fill);

// Reads an image file: .png, .jpg, .bmp or .gif (first frame).
Image load(const std::string& filename);

// Writes the image to a file. The format comes from the extension: .png,
// .jpg or .bmp. PNG and BMP keep transparency; JPEG does not.
void save(const Image& img, const std::string& filename);

// The color of the pixel at (row, col); the same as img[row][col].
Color getPixel(const Image& img, int row, int col);

// Changes the color of the pixel at (row, col); the same as
// img[row][col] = color.
void setPixel(Image& img, int row, int col, Color color);

// ---------------------------------------------------------------------------
// Transformations
//
// Each returns a new image and leaves the original unchanged:
//     image::Image small = image::resize(photo, photo.width / 2, photo.height / 2);
// ---------------------------------------------------------------------------

// The image mirrored left to right, or upside down.
Image flipHorizontal(const Image& img);
Image flipVertical(const Image& img);

// The image turned counterclockwise by degrees (clockwise if negative). The
// result is just big enough to hold the turned image, and the corners around
// it are transparent. Multiples of 90 degrees turn the pixels exactly.
Image rotate(const Image& img, double degrees);

// The image stretched or shrunk to width x height pixels, smoothly.
Image resize(const Image& img, int width, int height);

// The width x height part of the image whose top-left corner is the pixel at
// (row, col). It must lie inside the image.
Image crop(const Image& img, int row, int col, int width, int height);

}  // namespace image

#endif  // CANVAS_IMAGE_HPP
