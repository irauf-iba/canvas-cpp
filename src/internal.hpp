// internal.hpp - shared by the library's source files; not part of the API.

#ifndef CANVAS_INTERNAL_HPP
#define CANVAS_INTERNAL_HPP

#include <string>

#include "image.hpp"

namespace canvas_internal {

// --- common.cpp ---------------------------------------------------------------

// Prints "module: message" (e.g. "canvas: circle: radius must not be
// negative") and exits with status 1. An open window closes rather than
// waiting for the user, and queued sound is not finished.
[[noreturn]] void fail(const char* module, const std::string& message);

// True once fail() has been called.
bool failing();

// Called at the start of each module's exit handler. From then on nothing
// may call exit(), which is not allowed while the program is exiting.
void beginShutdown();
bool shuttingDown();

// True if the environment variable CANVAS_HEADLESS is set (and not "0").
bool headlessRequested();

// Where to open a file the program reads: the name as given, which is relative
// to the current folder, or else the same name in the program's own folder
// (IDEs often run programs from a different folder). "" if neither exists.
std::string findInputFile(const std::string& filename);

// The message for a file findInputFile() didn't find, naming the folders
// that were searched: "cannot open 'cat.png' (not in the current folder, ...)".
std::string notFound(const std::string& filename);

// The file name's extension in lower case, without the dot ("" if none).
std::string lowerExtension(const std::string& filename);

// --- image.cpp ----------------------------------------------------------------

// Stops with an error naming module and function unless the image's pixels
// match its width and height.
void checkImage(const image::Image& img, const char* module, const char* function);

// Reads or writes an image file; errors name module and function, e.g.
// "canvas: picture: cannot open 'x.png' (...)".
image::Image readImageFile(const std::string& filename, const char* module, const char* function);
void writeImageFile(const image::Image& img, const std::string& filename, const char* module,
                    const char* function);

// --- canvas.cpp -----------------------------------------------------------------

// If the canvas window is open, lets the OS know it is still responding and
// ends the program if it was closed. For code that waits outside canvas, such
// as audio::play(). Cheap to call often.
void keepWindowAlive();

// The centre of the canvas in user coordinates (with the current scale).
void canvasCenter(double& x, double& y);

// What the window shows: the canvas with the grid and watched values over it,
// at logical size. For tests; snapshot() never includes the overlay.
image::Image screenImage();

// --- recording.cpp ----------------------------------------------------------------

// Recording the canvas as a GIF. recordingStart() checks the file name and
// stops with an error naming startRecording. Each frame is given with the
// time, in seconds since the recording started, at which it appears; frames
// that don't change the picture are merged. The recording ends by itself
// after 60 seconds.
bool recordingActive();
void recordingStart(const std::string& filename);
void recordingFrame(const image::Image& frame, double seconds);

// Ends the recording with the last frame and writes the file. If writing
// fails, stops with an error (canFail) or, while the program is exiting,
// prints the problem.
void recordingFinish(const image::Image& last, double seconds, bool canFail);

}  // namespace canvas_internal

#endif  // CANVAS_INTERNAL_HPP
