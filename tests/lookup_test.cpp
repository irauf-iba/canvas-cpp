// Files the program reads are also found next to the program when they are
// not in the current folder, as when an IDE runs the program from another
// folder. CTest runs this from the build folder while the program is in
// tests/ (or tests/<config>/), so only that fallback can find the files.
//
// Usage: lookup_test <folder-of-this-program>

#include <audio.hpp>
#include <canvas.hpp>
#include <image.hpp>

#include <string>

#include "check.hpp"

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    const std::string programFolder = std::string(argv[1]) + "/";

    // Write the files next to the program (saving uses the path as given).
    image::save(image::create(3, 2, image::RED), programFolder + "lookup_test.png");
    audio::save(programFolder + "lookup_test.wav", {0.0, 0.5, -0.5});

    // Read them by bare name.
    image::Image img = image::load("lookup_test.png");
    CHECK(img.width == 3 && img.height == 2 && image::get(img, 0, 0) == image::RED);
    CHECK(audio::read("lookup_test.wav").size() == 3);
    canvas::picture(0.5, 0.5, "lookup_test.png");  // canvas uses the same lookup
    return finish();
}
