// A paddle game: keep the ball in play with the mouse. Shows a button with
// isMouseOver, a ball hitting a paddle, sound effects, colours from hsv(),
// and the debug keys: P pauses, N steps one frame, G shows the grid.
//
//     ./paddle            play
//     ./paddle record     also record the game to paddle.gif

#include <audio.hpp>
#include <canvas.hpp>

#include <algorithm>
#include <string>
#include <vector>

// True if a ball touches a paddle (a rectangle given by its centre, half
// width and half height): the point of the paddle nearest the ball's centre
// is no farther away than the radius.
bool touches(double x, double y, double radius, double padX, double padY, double halfWidth, double halfHeight) {
    double nearestX = std::clamp(x, padX - halfWidth, padX + halfWidth);
    double nearestY = std::clamp(y, padY - halfHeight, padY + halfHeight);
    return canvas::distance(x, y, nearestX, nearestY) <= radius;
}

int main(int argc, char** argv) {
    canvas::setTitle("Paddle");
    canvas::enableDoubleBuffering();
    canvas::setFrameRate(60);
    canvas::enableDebugKeys();
    if (argc > 1 && std::string(argv[1]) == "record") canvas::startRecording("paddle.gif");

    const std::vector<double> bounce = audio::tone(660, 0.05);
    const std::vector<double> missed = audio::note(-12, 0.4);  // A3, low and longer
    const double radius = 0.025, padY = 0.08, padHalfWidth = 0.1, padHalfHeight = 0.015;

    bool playing = false;
    int score = 0;
    double x = 0, y = 0, vx = 0, vy = 0;
    while (true) {
        canvas::clear(canvas::gray(30));
        if (!playing) {
            // A start button that lights up under the mouse.
            bool over = canvas::isMouseOver(0.5, 0.5, 0.15, 0.06);
            canvas::setPenColor(canvas::hsv(200, 0.6, over ? 1.0 : 0.7));
            canvas::filledRectangle(0.5, 0.5, 0.15, 0.06);
            canvas::setPenColor(canvas::WHITE);
            canvas::text(0.5, 0.5, "Start");
            if (score > 0) canvas::text(0.5, 0.65, "Score: " + std::to_string(score));
            if (over && canvas::mouseClicked()) {
                playing = true;
                score = 0;
                x = 0.5, y = 0.7, vx = 0.006, vy = 0.008;
            }
        } else {
            double padX = std::clamp(canvas::mouseX(), padHalfWidth, 1 - padHalfWidth);
            x += vx;
            y += vy;
            if ((x < radius && vx < 0) || (x > 1 - radius && vx > 0)) vx = -vx;
            if (y > 1 - radius && vy > 0) vy = -vy;
            if (vy < 0 && touches(x, y, radius, padX, padY, padHalfWidth, padHalfHeight)) {
                vy = -vy * 1.05;            // a little faster each time
                vx += (x - padX) * 0.05;    // the edge of the paddle sends it sideways
                ++score;
                audio::playInBackground(bounce);
            }
            if (y < -radius) {
                playing = false;
                audio::playInBackground(missed);
            }
            canvas::setPenColor(canvas::LIGHT_GRAY);
            canvas::filledRectangle(padX, padY, padHalfWidth, padHalfHeight);
            canvas::setPenColor(canvas::hsv(score * 35, 0.8, 1));  // a new colour for each point
            canvas::filledCircle(x, y, radius);
            canvas::setPenColor(canvas::WHITE);
            canvas::textLeft(0.03, 0.96, "Score: " + std::to_string(score));
        }
        canvas::show();
    }
}
