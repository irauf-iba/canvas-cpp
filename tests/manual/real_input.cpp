// Real-input test, driven by tests/manual/real_input.sh with xdotool.
//
// Runs a sequence of steps. Each step prints "WAIT <name>", waits for the
// expected input, then prints "PASS <name>" or "FAIL <name>". The script
// watches the output and sends each input only after the WAIT line appears.

#include <draw.hpp>

#include <cmath>
#include <cstdio>
#include <functional>
#include <string>

namespace {

int failures = 0;

// Waits up to timeoutMs for done() to become true, one frame at a time.
bool step(const char* name, const std::function<bool()>& done, int timeoutMs = 5000) {
    std::printf("WAIT %s\n", name);
    std::fflush(stdout);
    draw::clear();
    draw::text(0.5, 0.5, std::string("waiting for: ") + name);
    draw::show();
    for (int t = 0; t < timeoutMs; t += 10) {
        if (done()) {
            std::printf("PASS %s\n", name);
            std::fflush(stdout);
            return true;
        }
        draw::pause(10);
    }
    std::printf("FAIL %s (timed out)\n", name);
    std::fflush(stdout);
    ++failures;
    return false;
}

}  // namespace

int main() {
    draw::setTitle("draw real input test");
    draw::setCanvasSize(400, 300);
    draw::enableDoubleBuffering();

    // The script clicks the window to give it focus.
    step("start-click", [] { return draw::mouseClicked(); }, 15000);

    step("left-down", [] { return draw::isKeyPressed(draw::Key::Left); });
    step("left-up", [] { return !draw::isKeyPressed(draw::Key::Left); });
    step("a-down", [] { return draw::isKeyPressed(draw::Key::A); });
    step("a-up", [] { return !draw::isKeyPressed(draw::Key::A); });
    step("shift-down", [] { return draw::isKeyPressed(draw::Key::Shift); });
    step("shift-up", [] { return !draw::isKeyPressed(draw::Key::Shift); });
    step("space-down", [] { return draw::isKeyPressed(draw::Key::Space); });
    step("space-up", [] { return !draw::isKeyPressed(draw::Key::Space); });

    // Typed text, including a shifted character, Backspace and Enter. Keys
    // carry over between frames here only because each frame reads them all.
    std::string typed;
    step("typing", [&] {
        while (draw::hasNextKeyTyped()) typed += draw::nextKeyTyped();
        return typed.find('\n') != std::string::npos;
    });
    const std::string expected = "Hi!x\b\n";
    bool typedOk = typed == expected;
    std::printf("%s typed-text (got \"", typedOk ? "PASS" : "FAIL");
    for (char c : typed) {
        if (c == '\n') std::printf("\\n");
        else if (c == '\b') std::printf("\\b");
        else std::printf("%c", c);
    }
    std::printf("\")\n");
    if (!typedOk) ++failures;

    // The script moves the mouse to 75% across and 25% down the window,
    // which is (0.75, 0.75) in the default coordinates (y points up).
    step("mouse-move", [] {
        return std::abs(draw::mouseX() - 0.75) < 0.02 && std::abs(draw::mouseY() - 0.75) < 0.02;
    });
    step("mouse-down", [] { return draw::isMousePressed(); });
    step("mouse-up", [] { return !draw::isMousePressed(); });
    // A quick click (down and up within one frame) must not be missed.
    step("quick-click", [] { return draw::mouseClicked(); });

    std::printf("RESULT %d failure(s)\n", failures);
    std::printf("WAIT close\n");
    std::fflush(stdout);
    // The script now closes the window; that should end the program here.
    draw::pause(10000);
    std::printf("FAIL close (still running)\n");
    return 1;
}
