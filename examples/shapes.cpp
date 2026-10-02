// Draws one of each kind of shape.

#include <canvas.hpp>

int main() {
    canvas::setTitle("Shapes");

    canvas::setPenColor(canvas::BOOK_BLUE);
    canvas::filledCircle(0.2, 0.8, 0.1);
    canvas::filledSquare(0.5, 0.8, 0.1);
    canvas::filledRectangle(0.8, 0.8, 0.12, 0.06);

    canvas::setPenColor(canvas::BOOK_RED);
    canvas::setPenWidth(4);
    canvas::circle(0.2, 0.5, 0.1);
    canvas::ellipse(0.5, 0.5, 0.12, 0.06);
    canvas::arc(0.8, 0.5, 0.1, 0, 270);

    canvas::setPenColor(canvas::rgb(40, 160, 60));
    canvas::filledPolygon({{0.1, 0.1}, {0.3, 0.1}, {0.2, 0.3}});
    canvas::line(0.4, 0.1, 0.6, 0.3);
    canvas::polyline({{0.7, 0.1}, {0.75, 0.3}, {0.8, 0.1}, {0.85, 0.3}, {0.9, 0.1}});

    canvas::setPenColor(canvas::BLACK);
    canvas::text(0.5, 0.95, "Hello, canvas!");
}
