// A starting point: a ball that follows the mouse. Replace it with your own
// program.

#include <canvas.hpp>

int main() {
    canvas::setTitle("My program");
    canvas::enableDoubleBuffering();

    while (true) {
        canvas::clear();
        canvas::setPenColor(canvas::BOOK_BLUE);
        canvas::filledCircle(canvas::mouseX(), canvas::mouseY(), 0.05);
        canvas::setPenColor(canvas::BLACK);
        canvas::text(0.5, 0.95, "Move the mouse. Close the window to quit.");
        canvas::show();
        canvas::pause(16);
    }
}
