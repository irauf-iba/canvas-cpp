// internal.hpp - shared by the library's source files; not part of the API.

#ifndef DRAW_INTERNAL_HPP
#define DRAW_INTERNAL_HPP

#include <string>

#include "image.hpp"

namespace draw_internal {

// Prints "module: message" (e.g. "draw: circle: radius must not be
// negative") and exits with status 1. An open window closes rather than
// waiting for the user.
[[noreturn]] void fail(const char* module, const std::string& message);

// True once fail() has been called.
bool failing();

// Stops with an error naming module and function unless the image's pixels
// match its width and height.
void checkImage(const image::Image& img, const char* module, const char* function);

// Reads or writes an image file; errors name module and function, e.g.
// "draw: picture: cannot open 'x.png' (...)".
image::Image readImageFile(const std::string& filename, const char* module, const char* function);
void writeImageFile(const image::Image& img, const std::string& filename, const char* module,
                    const char* function);

}  // namespace draw_internal

#endif  // DRAW_INTERNAL_HPP
