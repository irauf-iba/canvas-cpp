// Flips n coins and counts the heads, many times over, then draws a
// histogram of how often each number of heads came up, together with the
// normal (Gaussian) curve that the histogram approaches.
//
//     ./bernoulli            (20 coins, 10000 times)
//     ./bernoulli 50 100000
//
// After Bernoulli in Sedgewick & Wayne, Computer Science: An Interdisciplinary
// Approach, Section 2.2. Written for this library.

#include <canvas.hpp>
#include <chance.hpp>
#include <input.hpp>

#include <cmath>
#include <vector>

int main(int argc, char** argv) {
    const double PI = 3.14159265358979323846;
    int n = argc > 1 ? input::toInt(argv[1]) : 20;
    int trials = argc > 2 ? input::toInt(argv[2]) : 10000;

    // counts[k]: how many times exactly k of the n coins came up heads.
    std::vector<int> counts(n + 1, 0);
    for (int t = 0; t < trials; ++t) {
        int heads = 0;
        for (int i = 0; i < n; ++i) {
            if (chance::bernoulli(0.5)) ++heads;
        }
        ++counts[heads];
    }

    // The normal curve with the same mean and standard deviation.
    double mean = n / 2.0, sd = std::sqrt(n) / 2;
    double peak = 1 / (sd * std::sqrt(2 * PI));

    canvas::setTitle("Bernoulli");
    canvas::setXscale(-1, n + 1);
    canvas::setYscale(-0.1 * peak, 1.25 * peak);
    canvas::setPenColor(canvas::BOOK_LIGHT_BLUE);
    for (int k = 0; k <= n; ++k) {
        double fraction = static_cast<double>(counts[k]) / trials;
        canvas::filledRectangle(k, fraction / 2, 0.45, fraction / 2);
    }

    std::vector<double> x, y;
    for (double k = 0; k <= n; k += n / 200.0) {
        x.push_back(k);
        y.push_back(std::exp(-(k - mean) * (k - mean) / (2 * sd * sd)) * peak);
    }
    canvas::setPenColor(canvas::BOOK_RED);
    canvas::setPenWidth(2);
    canvas::polyline(x, y);

    canvas::setPenColor(canvas::BLACK);
    canvas::text(mean, 1.15 * peak, std::to_string(n) + " coins, " + std::to_string(trials) + " times");
}
