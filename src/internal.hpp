// internal.hpp - shared by the library's source files; not part of the API.

#ifndef DRAW_INTERNAL_HPP
#define DRAW_INTERNAL_HPP

#include <string>

#include "image.hpp"

namespace draw_internal {

// --- common.cpp ---------------------------------------------------------------

// Prints "module: message" (e.g. "draw: circle: radius must not be
// negative") and exits with status 1. An open window closes rather than
// waiting for the user, and queued sound is not finished.
[[noreturn]] void fail(const char* module, const std::string& message);

// True once fail() has been called.
bool failing();

// True if the environment variable DRAW_HEADLESS is set (and not "0").
bool headlessRequested();

// The file name's extension in lower case, without the dot ("" if none).
std::string lowerExtension(const std::string& filename);

// --- image.cpp ----------------------------------------------------------------

// Stops with an error naming module and function unless the image's pixels
// match its width and height.
void checkImage(const image::Image& img, const char* module, const char* function);

// Reads or writes an image file; errors name module and function, e.g.
// "draw: picture: cannot open 'x.png' (...)".
image::Image readImageFile(const std::string& filename, const char* module, const char* function);
void writeImageFile(const image::Image& img, const std::string& filename, const char* module,
                    const char* function);

// --- draw.cpp -----------------------------------------------------------------

// If the draw window is open, lets the OS know it is still responding and
// ends the program if it was closed. For code that waits outside draw, such
// as audio::play(). Cheap to call often.
void keepWindowAlive();

}  // namespace draw_internal

#endif  // DRAW_INTERNAL_HPP
