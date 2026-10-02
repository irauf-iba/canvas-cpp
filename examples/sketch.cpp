// Drag the mouse to draw. Click to drop a dot, type r/g/b to change color,
// c to clear, and s to save a picture.

#include <draw.hpp>

int main() {
    draw::setTitle("Sketch");
    draw::setPenWidth(3);

    double lastX = 0, lastY = 0;
    bool wasPressed = false;

    while (true) {
        while (draw::hasNextKeyTyped()) {
            char c = draw::nextKeyTyped();
            if (c == 'r') draw::setPenColor(draw::RED);
            if (c == 'g') draw::setPenColor(draw::GREEN);
            if (c == 'b') draw::setPenColor(draw::BLUE);
            if (c == 'c') draw::clear();
            if (c == 's') draw::save("sketch.png");
        }

        double x = draw::mouseX(), y = draw::mouseY();
        if (draw::mouseClicked()) draw::filledCircle(x, y, 0.01);

        if (draw::isMousePressed()) {
            if (wasPressed) draw::line(lastX, lastY, x, y);
            lastX = x;
            lastY = y;
        }
        wasPressed = draw::isMousePressed();

        draw::pause(10);
    }
}
