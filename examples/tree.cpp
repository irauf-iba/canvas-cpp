// A tree drawn by recursion: a trunk with two smaller trees on top, each a
// trunk with two smaller trees, and so on.

#include <canvas.hpp>
#include <turtle.hpp>

// Draws a tree from where the turtle is, in the direction it faces, and
// leaves the turtle exactly where it started. That is what lets each branch
// grow the next ones.
void tree(int depth, double length) {
    if (depth == 0) return;
    canvas::setPenWidth(depth);
    canvas::setPenColor(depth > 2 ? canvas::BROWN : canvas::rgb(40, 150, 60));
    turtle::forward(length);

    turtle::turnLeft(25);
    tree(depth - 1, length * 0.72);   // the left branch
    turtle::turnRight(50);
    tree(depth - 1, length * 0.72);   // the right branch
    turtle::turnLeft(25);

    turtle::penUp();                  // back down the trunk, without drawing
    turtle::backward(length);
    turtle::penDown();
}

int main() {
    canvas::setTitle("Tree");
    turtle::penUp();
    turtle::moveTo(0.5, 0.05);
    turtle::setHeading(90);           // up
    turtle::penDown();
    tree(10, 0.25);
}
