// A small piano: type the keys a s d f g h j k to play a C major scale, or
// click the keys. Notes play in the background, so several can sound at once.

#include <audio.hpp>
#include <canvas.hpp>

#include <cmath>
#include <string>
#include <vector>

const double PI = 3.14159265358979323846;
const std::string KEYS = "asdfghjk";
const int SEMITONES[] = {0, 2, 4, 5, 7, 9, 11, 12};
const int N = 8;  // number of keys

// A plucked-sounding note: a tone with a quick fade-out.
std::vector<double> note(int semitones) {
    double hz = 261.63 * std::pow(2.0, semitones / 12.0);
    int n = audio::SAMPLE_RATE;  // one second
    std::vector<double> samples(n);
    for (int i = 0; i < n; ++i) {
        double t = static_cast<double>(i) / audio::SAMPLE_RATE;
        samples[i] = 0.3 * std::exp(-4 * t) * std::sin(2 * PI * hz * t);
    }
    return samples;
}

int main() {
    std::vector<double> notes[N];
    for (int i = 0; i < N; ++i) notes[i] = note(SEMITONES[i]);
    int lit[N] = {};  // frames left to show each key as pressed

    canvas::setTitle("Piano");
    canvas::setCanvasSize(640, 200);
    canvas::setXscale(0, N);
    canvas::enableDoubleBuffering();
    canvas::setFrameRate(60);

    while (true) {
        int pressed = -1;
        while (canvas::hasNextKeyTyped()) {
            std::size_t k = KEYS.find(canvas::nextKeyTyped());
            if (k != std::string::npos) pressed = static_cast<int>(k);
        }
        if (canvas::mouseClicked()) pressed = static_cast<int>(canvas::mouseX());
        if (pressed >= 0 && pressed < N) {
            audio::playInBackground(notes[pressed]);
            lit[pressed] = 10;
        }

        canvas::clear(canvas::DARK_GRAY);
        for (int i = 0; i < N; ++i) {
            canvas::setPenColor(lit[i] > 0 ? canvas::BOOK_LIGHT_BLUE : canvas::WHITE);
            canvas::filledRectangle(i + 0.5, 0.5, 0.45, 0.45);
            canvas::setPenColor(canvas::BLACK);
            canvas::text(i + 0.5, 0.15, std::string(1, KEYS[i]));
            if (lit[i] > 0) --lit[i];
        }
        canvas::show();
    }
}
