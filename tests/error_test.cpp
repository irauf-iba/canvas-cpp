// Each case makes one mistake a student might make. The test passes when the
// program stops with the expected message (checked by CTest).
//
// Usage: error_test <case>

#include <draw.hpp>

#include <cmath>
#include <cstdio>
#include <functional>
#include <map>
#include <string>

int main(int argc, char** argv) {
    const std::map<std::string, std::function<void()>> cases = {
        {"nan", [] { draw::circle(0.5, std::sqrt(-1.0), 0.1); }},
        {"negative-radius", [] { draw::filledCircle(0.5, 0.5, -1); }},
        {"missing-picture", [] { draw::picture(0.5, 0.5, "no-such-file.png"); }},
        {"bad-save-format", [] { draw::save("out.gif"); }},
        {"size-mismatch", [] { draw::polygon({0.1, 0.2}, {0.1}); }},
        {"no-key", [] { draw::nextKeyTyped(); }},
        {"empty-scale", [] { draw::setXscale(1, 1); }},
        {"bad-canvas-size", [] { draw::setCanvasSize(0, 100); }},
        {"missing-font", [] { draw::setFont("no-such-font.ttf"); }},
        {"negative-pause", [] { draw::pause(-1); }},
    };
    if (argc != 2 || cases.count(argv[1]) == 0) {
        std::printf("usage: error_test <case>\n");
        return 2;
    }
    cases.at(argv[1])();
    std::printf("no error was reported\n");
    return 0;
}
