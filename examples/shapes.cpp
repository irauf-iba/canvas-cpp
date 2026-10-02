// Draws one of each kind of shape.

#include <draw.hpp>

int main() {
    draw::setTitle("Shapes");

    draw::setPenColor(draw::BOOK_BLUE);
    draw::filledCircle(0.2, 0.8, 0.1);
    draw::filledSquare(0.5, 0.8, 0.1);
    draw::filledRectangle(0.8, 0.8, 0.12, 0.06);

    draw::setPenColor(draw::BOOK_RED);
    draw::setPenWidth(4);
    draw::circle(0.2, 0.5, 0.1);
    draw::ellipse(0.5, 0.5, 0.12, 0.06);
    draw::arc(0.8, 0.5, 0.1, 0, 270);

    draw::setPenColor(draw::rgb(40, 160, 60));
    draw::filledPolygon({{0.1, 0.1}, {0.3, 0.1}, {0.2, 0.3}});
    draw::line(0.4, 0.1, 0.6, 0.3);
    draw::polyline({{0.7, 0.1}, {0.75, 0.3}, {0.8, 0.1}, {0.85, 0.3}, {0.9, 0.1}});

    draw::setPenColor(draw::BLACK);
    draw::text(0.5, 0.95, "Hello, draw!");
}
