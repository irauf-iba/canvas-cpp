# Changelog

Projects choose a version with `GIT_TAG` in their `CMakeLists.txt` (the
[template](template) uses the latest release). Releases don't change within a
term; a new version can rename or remove functions, and lists what changed
here.

## v0.1 (2026-10-04)

The first release, for an introductory C++ course. Modules, each with a guide
in [docs](docs):

- **canvas**: drawing, text, pictures, animation with a frame rate, mouse and
  keyboard input per frame, and recording a GIF. Debugging aids: hints for
  likely mistakes, a coordinate grid, the mouse position in the title,
  watched values, slow motion and debug keys.
- **turtle**: turtle graphics on the canvas.
- **image**: images as grids of pixels, `img[row][col]`, loading, saving and
  cropping.
- **audio**: sound as samples, WAV and MP3 files, background sound, tones and
  notes.
- **chance**: random numbers, the same on every computer for a given seed.
- **stats**: mean, median, standard deviation and more.
- **stopwatch**: timing code.
- **input** and **output**: help with `std::cin` and `std::cout`, files in an
  IDE.

For autograding: headless mode (`CANVAS_HEADLESS=1`), captured sound
(`CANVAS_AUDIO_CAPTURE`), repeatable random numbers (`CANVAS_SEED`), and the
same pixels on every computer.

Tested by CI on Linux, Windows (MSVC) and macOS, in Debug and Release.
