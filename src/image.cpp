// image.cpp - implementation of image.hpp, plus the image file reading and
// writing used by canvas.cpp. Needs no window and does not use SDL.

#include "image.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

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
// Transformations
// ---------------------------------------------------------------------------

namespace image {
namespace {

[[noreturn]] void fail(const char* function, const std::string& message) {
    canvas_internal::fail("image", std::string(function) + ": " + message);
}

std::size_t at(int width, int row, int col) {
    return static_cast<std::size_t>(row) * static_cast<std::size_t>(width) + static_cast<std::size_t>(col);
}

Image blank(int width, int height) { return create(width, height, Color{0, 0, 0, 0}); }

// A color with premultiplied alpha, for filtering: averaging premultiplied
// colors keeps transparent pixels from darkening their neighbours.
struct Sum {
    double r = 0, g = 0, b = 0, a = 0;
    void add(Color c, double weight) {
        const double w = weight * c.a;
        r += c.r * w;
        g += c.g * w;
        b += c.b * w;
        a += w;
    }
    Color color() const {
        if (a <= 0) return Color{0, 0, 0, 0};
        auto byte = [](double v) { return static_cast<std::uint8_t>(std::lround(std::clamp(v, 0.0, 255.0))); };
        return Color{byte(r / a), byte(g / a), byte(b / a), byte(a)};
    }
};

// The weights with which source pixels 0..n-1 make up output pixel i, when
// n pixels are resampled to m: an average over the pixels it covers when
// shrinking, linear interpolation between the two nearest when enlarging.
std::vector<std::pair<int, double>> weights(int i, int n, int m) {
    std::vector<std::pair<int, double>> w;
    const double scale = static_cast<double>(n) / m;
    if (scale > 1) {
        const double x0 = i * scale, x1 = (i + 1) * scale;
        for (int k = static_cast<int>(std::floor(x0)); k < static_cast<int>(std::ceil(x1)) && k < n; ++k) {
            const double cover = std::min(x1, k + 1.0) - std::max(x0, static_cast<double>(k));
            if (cover > 0) w.emplace_back(k, cover / scale);
        }
    } else {
        const double x = (i + 0.5) * scale - 0.5;
        const int k = static_cast<int>(std::floor(x));
        const double f = x - k;
        w.emplace_back(std::clamp(k, 0, n - 1), 1 - f);
        w.emplace_back(std::clamp(k + 1, 0, n - 1), f);
    }
    return w;
}

}  // namespace

Image flipHorizontal(const Image& img) {
    canvas_internal::checkImage(img, "image", "flipHorizontal");
    Image out = img;
    for (int row = 0; row < img.height; ++row) {
        for (int col = 0; col < img.width; ++col) {
            out.pixels[at(img.width, row, col)] = img.pixels[at(img.width, row, img.width - 1 - col)];
        }
    }
    return out;
}

Image flipVertical(const Image& img) {
    canvas_internal::checkImage(img, "image", "flipVertical");
    Image out = img;
    for (int row = 0; row < img.height; ++row) {
        for (int col = 0; col < img.width; ++col) {
            out.pixels[at(img.width, row, col)] = img.pixels[at(img.width, img.height - 1 - row, col)];
        }
    }
    return out;
}

Image rotate(const Image& img, double degrees) {
    canvas_internal::checkImage(img, "image", "rotate");
    if (!std::isfinite(degrees)) fail("rotate", "degrees is NaN or infinite");
    const int w = img.width, h = img.height;
    double turn = std::fmod(degrees, 360.0);
    if (turn < 0) turn += 360;

    // Quarter turns move pixels without filtering.
    if (turn == 0) return img;
    if (turn == 90 || turn == 270) {
        Image out = blank(h, w);
        for (int row = 0; row < w; ++row) {
            for (int col = 0; col < h; ++col) {
                out.pixels[at(h, row, col)] = turn == 90 ? img.pixels[at(w, col, w - 1 - row)]
                                                         : img.pixels[at(w, h - 1 - col, row)];
            }
        }
        return out;
    }
    if (turn == 180) {
        Image out = blank(w, h);
        for (std::size_t i = 0; i < img.pixels.size(); ++i) out.pixels[i] = img.pixels[img.pixels.size() - 1 - i];
        return out;
    }

    // Other angles: each output pixel is read from where it came from in the
    // source, between pixels, with transparency outside it (which also
    // smooths the edges).
    const double pi = 3.14159265358979323846;
    const double c = std::cos(turn * pi / 180), s = std::sin(turn * pi / 180);
    const int outW = static_cast<int>(std::ceil(std::abs(w * c) + std::abs(h * s) - 1e-6));
    const int outH = static_cast<int>(std::ceil(std::abs(w * s) + std::abs(h * c) - 1e-6));
    Image out = blank(outW, outH);
    auto texel = [&](int x, int y) {
        return x < 0 || y < 0 || x >= w || y >= h ? Color{0, 0, 0, 0} : img.pixels[at(w, y, x)];
    };
    for (int row = 0; row < outH; ++row) {
        for (int col = 0; col < outW; ++col) {
            // Relative to the centre, with y down: turning counterclockwise
            // on screen, so the source is found by turning back.
            const double dx = col + 0.5 - outW / 2.0, dy = row + 0.5 - outH / 2.0;
            const double u = dx * c - dy * s + w / 2.0 - 0.5, v = dx * s + dy * c + h / 2.0 - 0.5;
            const int x = static_cast<int>(std::floor(u)), y = static_cast<int>(std::floor(v));
            const double fu = u - x, fv = v - y;
            Sum sum;
            sum.add(texel(x, y), (1 - fu) * (1 - fv));
            sum.add(texel(x + 1, y), fu * (1 - fv));
            sum.add(texel(x, y + 1), (1 - fu) * fv);
            sum.add(texel(x + 1, y + 1), fu * fv);
            out.pixels[at(outW, row, col)] = sum.color();
        }
    }
    return out;
}

Image resize(const Image& img, int width, int height) {
    canvas_internal::checkImage(img, "image", "resize");
    if (width <= 0 || height <= 0) fail("resize", "width and height must be positive");
    if (img.width == 0 || img.height == 0) fail("resize", "the image is empty");
    // Columns first, then rows.
    std::vector<std::vector<std::pair<int, double>>> across, down;
    for (int col = 0; col < width; ++col) across.push_back(weights(col, img.width, width));
    for (int row = 0; row < height; ++row) down.push_back(weights(row, img.height, height));

    std::vector<Sum> middle(static_cast<std::size_t>(width) * static_cast<std::size_t>(img.height));
    for (int row = 0; row < img.height; ++row) {
        for (int col = 0; col < width; ++col) {
            Sum& sum = middle[at(width, row, col)];
            for (const auto& [k, weight] : across[static_cast<std::size_t>(col)]) {
                sum.add(img.pixels[at(img.width, row, k)], weight);
            }
        }
    }
    Image out = blank(width, height);
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            Sum sum;
            for (const auto& [k, weight] : down[static_cast<std::size_t>(row)]) {
                const Sum& m = middle[at(width, k, col)];  // already premultiplied
                sum.r += m.r * weight;
                sum.g += m.g * weight;
                sum.b += m.b * weight;
                sum.a += m.a * weight;
            }
            out.pixels[at(width, row, col)] = sum.color();
        }
    }
    return out;
}

Image crop(const Image& img, int row, int col, int width, int height) {
    canvas_internal::checkImage(img, "image", "crop");
    if (width < 0 || height < 0) fail("crop", "width and height must not be negative");
    if (row < 0 || col < 0 || row + height > img.height || col + width > img.width) {
        fail("crop", "the " + std::to_string(width) + " x " + std::to_string(height) + " part at row " +
                         std::to_string(row) + ", col " + std::to_string(col) + " is not inside the " +
                         std::to_string(img.width) + " x " + std::to_string(img.height) + " image");
    }
    Image out = blank(width, height);
    for (int r = 0; r < height; ++r) {
        for (int c = 0; c < width; ++c) out.pixels[at(width, r, c)] = img.pixels[at(img.width, row + r, col + c)];
    }
    return out;
}

}  // namespace image
