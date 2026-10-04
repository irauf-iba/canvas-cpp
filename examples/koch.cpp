// A Koch snowflake: each side is replaced by four sides a third as long,
// with a bump in the middle, over and over. Asks for the depth first.

#include <canvas.hpp>
#include <input.hpp>
#include <turtle.hpp>

// Draws one side of the snowflake: a straight line at depth 0, or four
// smaller Koch curves with turns between them.
void koch(int depth, double size) {
    if (depth == 0) {
        turtle::forward(size);
        return;
    }
    koch(depth - 1, size / 3);
    turtle::turnLeft(60);
    koch(depth - 1, size / 3);
    turtle::turnRight(120);
    koch(depth - 1, size / 3);
    turtle::turnLeft(60);
    koch(depth - 1, size / 3);
}

int main() {
    int depth = input::askInt("Depth (0 to 6)? ", 0, 6);

    canvas::setTitle("Koch snowflake");
    canvas::setPenColor(canvas::BOOK_BLUE);
    turtle::penUp();
    turtle::moveTo(0.2, 0.68);  // the top left corner of the triangle
    turtle::penDown();
    for (int side = 0; side < 3; ++side) {
        koch(depth, 0.6);
        turtle::turnRight(120);
    }
}
