// image.hpp - images as grids of pixels, for image-processing exercises.
//
// Inspired by Princeton's Picture class. An Image is a plain struct you can
// copy, pass to functions and return from them; the functions below create,
// load, save, read and change it. No window is needed. To show an image, use
// draw::picture() from draw.hpp.
//
//     image::Image src = image::load("photo.png");
//     image::Image out = image::create(src.width, src.height);
//     for (int row = 0; row < src.height; ++row) {
//         for (int col = 0; col < src.width; ++col) {
//             image::Color c = image::get(src, col, row);
//             int gray = (299 * c.r + 587 * c.g + 114 * c.b) / 1000;
//             image::set(out, col, row, image::rgb(gray, gray, gray));
//         }
//     }
//     image::save(out, "gray.png");
//
// Pixels are addressed by column and row. Column 0 is at the left and row 0
// at the top, as in image files and image editors. (This differs from draw,
// where y points up, because rows and columns are positions in a grid, not
// coordinates.)
//
// Errors, such as a missing file or a pixel outside the image, print a
// message and stop the program.

#ifndef DRAW_IMAGE_HPP
#define DRAW_IMAGE_HPP

#include <string>
#include <vector>

#include "color.hpp"

namespace image {

// An image of width x height pixels. pixels holds the rows one after
// another, top row first: pixel (col, row) is pixels[row * width + col].
// Prefer get() and set(), which check that (col, row) is inside the image.
struct Image {
    int width = 0;
    int height = 0;
    std::vector<Color> pixels;
};

// A new image filled with the color (default WHITE).
Image create(int width, int height);
Image create(int width, int height, Color fill);

// Reads an image file: .png, .jpg, .bmp or .gif (first frame).
Image load(const std::string& filename);

// Writes the image to a file. The format comes from the extension: .png,
// .jpg or .bmp. PNG and BMP keep transparency; JPEG does not.
void save(const Image& img, const std::string& filename);

// The color of pixel (col, row).
Color get(const Image& img, int col, int row);

// Changes the color of pixel (col, row).
void set(Image& img, int col, int row, Color color);

}  // namespace image

#endif  // DRAW_IMAGE_HPP
