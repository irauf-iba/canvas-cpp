// Each case makes one mistake a student might make. The test passes when the
// program stops with the expected message (checked by CTest).
//
// Usage: error_test <case>

#include <audio.hpp>
#include <canvas.hpp>
#include <chance.hpp>
#include <image.hpp>
#include <input.hpp>
#include <output.hpp>
#include <turtle.hpp>

#include <cmath>
#include <cstdio>
#include <fstream>
#include <functional>
#include <map>
#include <sstream>
#include <string>

int main(int argc, char** argv) {
    const std::map<std::string, std::function<void()>> cases = {
        {"nan", [] { canvas::circle(0.5, std::sqrt(-1.0), 0.1); }},
        {"negative-radius", [] { canvas::filledCircle(0.5, 0.5, -1); }},
        {"missing-picture", [] { canvas::picture(0.5, 0.5, "no-such-file.png"); }},
        {"bad-save-format", [] { canvas::save("out.gif"); }},
        {"size-mismatch", [] { canvas::polygon({0.1, 0.2}, {0.1}); }},
        {"no-key", [] { canvas::nextKeyTyped(); }},
        {"empty-scale", [] { canvas::setXscale(1, 1); }},
        {"bad-canvas-size", [] { canvas::setCanvasSize(0, 100); }},
        {"missing-font", [] { canvas::setFont("no-such-font.ttf"); }},
        {"negative-pause", [] { canvas::pause(-1); }},
        {"negative-frame-rate", [] { canvas::setFrameRate(-30); }},
        {"grid-step", [] { canvas::showGrid(0); }},
        {"image-get-outside", [] { image::getPixel(image::create(5, 3), 0, 5); }},
        {"image-set-outside", [] { image::Image img = image::create(5, 3); image::setPixel(img, -1, 0, image::RED); }},
        {"image-index-outside", [] { image::Image img = image::create(5, 3); img[3][0] = image::RED; }},
        {"image-index-const", [] { const image::Image img = image::create(5, 3); image::Color c = img[0][-1]; (void)c; }},
        {"image-load-missing", [] { image::load("no-such-file.png"); }},
        {"image-create-negative", [] { image::create(-1, 10); }},
        {"image-save-format", [] { image::save(image::create(2, 2), "out.gif"); }},
        {"image-save-empty", [] { image::save(image::create(0, 0), "out.png"); }},
        {"audio-read-missing", [] { audio::read("no-such-file.wav"); }},
        {"audio-read-format", [] { audio::read("music.ogg"); }},
        {"audio-read-invalid", [] {
            std::ofstream("not-audio.wav") << "this is not a WAV file";
            audio::read("not-audio.wav");
        }},
        {"audio-save-format", [] { audio::save("out.mp3", {0.0}); }},
        {"audio-play-missing", [] { audio::play("no-such-file.mp3"); }},
        {"input-bad-int", [] {
            std::ofstream("bad.txt") << "1 2\n3 abc\n";
            input::fromFile("bad.txt");
            input::readAllInts();
        }},
        {"input-too-large", [] {
            std::ofstream("big.txt") << "99999999999\n";
            input::fromFile("big.txt");
            input::readAllInts();
        }},
        {"input-bad-double", [] {
            std::istringstream in("2.5\n\n1,5\n");  // a decimal comma
            input::readAllDoubles(in);
        }},
        {"input-ask-range", [] { input::askInt("Pick? ", 5, 1); }},
        {"input-to-int", [] { input::toInt("3.5"); }},
        {"input-to-double", [] { input::toDouble("abc"); }},
        {"input-missing-file", [] { input::fromFile("no-such-file.txt"); }},
        {"output-cannot-write", [] { output::toFile("no-such-folder/out.txt"); }},
        {"turtle-nan", [] { turtle::forward(std::sqrt(-1.0)); }},
        {"chance-empty-range", [] { chance::uniform(5, 5); }},
        {"chance-bad-n", [] { chance::uniform(0); }},
        {"chance-bad-p", [] { chance::bernoulli(1.5); }},
        {"chance-bad-sum", [] { chance::discrete(std::vector<double>{0.5, 0.4}); }},
        {"chance-bad-seed", [] { chance::uniform(); }},
        {"image-inconsistent", [] {
            image::Image img;
            img.width = 2;
            img.height = 2;
            img.pixels.resize(3);
            canvas::picture(0.5, 0.5, img);
        }},
    };
    if (argc != 2 || cases.count(argv[1]) == 0) {
        std::printf("usage: error_test <case>\n");
        return 2;
    }
    cases.at(argv[1])();
    std::printf("no error was reported\n");
    return 0;
}
