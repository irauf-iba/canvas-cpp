// Drag the mouse to draw. Click to drop a dot, type r/g/b to change color,
// c to clear, and s to save a picture.

#include <canvas.hpp>

int main() {
    canvas::setTitle("Sketch");
    canvas::setPenWidth(3);

    double lastX = 0, lastY = 0;
    bool wasPressed = false;

    while (true) {
        while (canvas::hasNextKeyTyped()) {
            char c = canvas::nextKeyTyped();
            if (c == 'r') canvas::setPenColor(canvas::RED);
            if (c == 'g') canvas::setPenColor(canvas::GREEN);
            if (c == 'b') canvas::setPenColor(canvas::BLUE);
            if (c == 'c') canvas::clear();
            if (c == 's') canvas::save("sketch.png");
        }

        double x = canvas::mouseX(), y = canvas::mouseY();
        if (canvas::mouseClicked()) canvas::filledCircle(x, y, 0.01);

        if (canvas::isMousePressed()) {
            if (wasPressed) canvas::line(lastX, lastY, x, y);
            lastX = x;
            lastY = y;
        }
        wasPressed = canvas::isMousePressed();

        canvas::pause(10);
    }
}
