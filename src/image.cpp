// image.cpp - implementation of image.hpp, plus the image file reading and
// writing used by canvas.cpp. Needs no window and does not use SDL.

#include "image.hpp"

#include <cstring>
#include <string>

#include "internal.hpp"

// stb libraries, compiled into this file with internal linkage so they cannot
// clash with a copy of stb in the student's own program.
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_BMP
#define STBI_ONLY_GIF
#define STBI_WINDOWS_UTF8
#include <stb_image.h>

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBIW_WINDOWS_UTF8
#include <stb_image_write.h>

// Pixels are read and written as raw RGBA bytes.
static_assert(sizeof(image::Color) == 4, "Color must be exactly 4 bytes");

namespace canvas_internal {

void checkImage(const image::Image& img, const char* module, const char* function) {
    if (img.width < 0 || img.height < 0 ||
        img.pixels.size() != static_cast<std::size_t>(img.width) * static_cast<std::size_t>(img.height)) {
        fail(module, std::string(function) + ": the image has " + std::to_string(img.pixels.size()) +
                         " pixels, but width x height is " + std::to_string(img.width) + " x " +
                         std::to_string(img.height));
    }
}

image::Image readImageFile(const std::string& filename, const char* module, const char* function) {
    const std::string path = findInputFile(filename);
    if (path.empty()) fail(module, std::string(function) + ": " + notFound(filename));
    int w, h, channels;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 4);
    if (!data) {
        fail(module, std::string(function) + ": cannot read '" + filename + "' (" +
                         stbi_failure_reason() + ")");
    }
    image::Image img;
    img.width = w;
    img.height = h;
    img.pixels.resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h));
    std::memcpy(img.pixels.data(), data, img.pixels.size() * 4);
    stbi_image_free(data);
    return img;
}

void writeImageFile(const image::Image& img, const std::string& filename, const char* module,
                    const char* function) {
    checkImage(img, module, function);
    if (img.width == 0 || img.height == 0) {
        fail(module, std::string(function) + ": cannot save an empty image");
    }
    const std::string ext = lowerExtension(filename);
    const void* data = img.pixels.data();
    int ok = 0;
    if (ext == "png") {
        ok = stbi_write_png(filename.c_str(), img.width, img.height, 4, data, img.width * 4);
    } else if (ext == "jpg" || ext == "jpeg") {
        ok = stbi_write_jpg(filename.c_str(), img.width, img.height, 4, data, 95);
    } else if (ext == "bmp") {
        ok = stbi_write_bmp(filename.c_str(), img.width, img.height, 4, data);
    } else {
        fail(module, std::string(function) + ": '" + filename + "' must end in .png, .jpg or .bmp");
    }
    if (!ok) fail(module, std::string(function) + ": cannot write '" + filename + "'");
}

}  // namespace canvas_internal

namespace image {

std::size_t detail::pixelIndex(const Image& img, int row, int col, const char* function) {
    canvas_internal::checkImage(img, "image", function);
    auto outside = [&](const char* name, int value, int size) {
        std::string range = size == 0 ? "the image is empty" : "0 to " + std::to_string(size - 1);
        canvas_internal::fail("image", std::string(function) + ": " + name + " " + std::to_string(value) +
                                           " is outside the image (" + range + ")");
    };
    if (row < 0 || row >= img.height) outside("row", row, img.height);
    if (col < 0 || col >= img.width) outside("col", col, img.width);
    return static_cast<std::size_t>(row) * static_cast<std::size_t>(img.width) + static_cast<std::size_t>(col);
}

Image create(int width, int height) { return create(width, height, WHITE); }

Image create(int width, int height, Color fill) {
    if (width < 0 || height < 0) {
        canvas_internal::fail("image", "create: width and height must not be negative");
    }
    Image img;
    img.width = width;
    img.height = height;
    img.pixels.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), fill);
    return img;
}

Image load(const std::string& filename) {
    return canvas_internal::readImageFile(filename, "image", "load");
}

void save(const Image& img, const std::string& filename) {
    canvas_internal::writeImageFile(img, filename, "image", "save");
}

Color getPixel(const Image& img, int row, int col) {
    return img.pixels[detail::pixelIndex(img, row, col, "getPixel")];
}

void setPixel(Image& img, int row, int col, Color color) {
    img.pixels[detail::pixelIndex(img, row, col, "setPixel")] = color;
}

}  // namespace image

// ---------------------------------------------------------------------------
// Cropping
// ---------------------------------------------------------------------------

namespace image {

Image crop(const Image& img, int row, int col, int rows, int cols) {
    canvas_internal::checkImage(img, "image", "crop");
    auto fail = [](const std::string& message) { canvas_internal::fail("image", "crop: " + message); };
    if (rows < 0 || cols < 0) fail("rows and cols must not be negative");
    if (row < 0 || col < 0 || row + rows > img.height || col + cols > img.width) {
        auto range = [](int from, int count) {
            return std::to_string(from) + " to " + std::to_string(from + count - 1);
        };
        fail("rows " + range(row, rows) + " and cols " + range(col, cols) +
             " are not all inside the image (rows " + range(0, img.height) + ", cols " + range(0, img.width) + ")");
    }
    Image out = create(cols, rows);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            out.pixels[static_cast<std::size_t>(r) * static_cast<std::size_t>(cols) + static_cast<std::size_t>(c)] =
                img.pixels[static_cast<std::size_t>(row + r) * static_cast<std::size_t>(img.width) +
                           static_cast<std::size_t>(col + c)];
        }
    }
    return out;
}

}  // namespace image
