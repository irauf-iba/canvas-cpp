// Tests for stats.hpp: the numbers, and where the plots draw.

#include <canvas.hpp>
#include <image.hpp>
#include <stats.hpp>

#include <cmath>
#include <vector>

#include "check.hpp"

namespace {

bool near(double a, double b) { return std::abs(a - b) < 1e-9; }

void testNumbers() {
    const std::vector<double> v = {2, 4, 4, 4, 5, 5, 7, 9};
    CHECK(near(stats::sum(v), 40));
    CHECK(near(stats::mean(v), 5));
    CHECK(near(stats::min(v), 2));
    CHECK(near(stats::max(v), 9));
    CHECK(near(stats::median(v), 4.5));                 // even count: the two middle values
    CHECK(near(stats::variance(v), 32.0 / 7));          // divided by n - 1
    CHECK(near(stats::stddev(v), std::sqrt(32.0 / 7)));

    CHECK(near(stats::median({3.5, -1, 2}), 2));        // odd count: the middle value
    CHECK(near(stats::median({7}), 7));
    CHECK(near(stats::sum(std::vector<double>{}), 0));  // an empty sum is 0
    CHECK(near(stats::min({-0.5, -3, 1e300}), -3));
    CHECK(near(stats::mean({1, 2, 6}), 3));             // whole numbers in braces work too

    // median sorts a copy, not the caller's vector.
    std::vector<double> unsorted = {9, 1, 5};
    stats::median(unsorted);
    CHECK((unsorted == std::vector<double>{9, 1, 5}));

    // Large values close together: two passes keep the variance exact.
    CHECK(near(stats::variance({1e9 + 4, 1e9 + 7, 1e9 + 13, 1e9 + 16}), 30));
}

void testInts() {
    const std::vector<int> v = {3, 1, 4, 1, 5, 9};
    CHECK(stats::sum(v) == 23);
    CHECK(stats::min(v) == 1);
    CHECK(stats::max(v) == 9);
    CHECK(near(stats::mean(v), 23.0 / 6));
    CHECK(near(stats::median(v), 3.5));                 // a median of ints can be a half
    CHECK(near(stats::variance(std::vector<int>{1, 2, 3, 4}), 5.0 / 3));
    CHECK(near(stats::stddev(std::vector<int>{1, 1, 1}), 0));

    // The sum of ints is a long long, past what an int holds.
    CHECK(stats::sum(std::vector<int>{2000000000, 2000000000}) == 4000000000LL);
}

bool isPen(const image::Image& img, int row, int col) {
    return image::getPixel(img, row, col) == canvas::BOOK_BLUE;
}

void testPlots() {
    canvas::setCanvasSize(300, 200);
    canvas::setPenColor(canvas::BOOK_BLUE);

    // x from -1 to 2, so the bars are at columns 100 and 200. y from -2.25
    // to 3.25 (the values with a 5% margin), so y = 0 is at row 200 * 3.25 / 5.5.
    stats::plotBars(std::vector<int>{-2, 3});
    image::Image img = canvas::snapshot();
    CHECK(isPen(img, 150, 100));   // the first bar goes down from 0 ...
    CHECK(!isPen(img, 100, 100));  // ... and not up
    CHECK(isPen(img, 60, 200));    // the second goes up
    CHECK(!isPen(img, 150, 200));
    CHECK(!isPen(img, 60, 150));   // nothing between the bars

    // Points and lines don't include 0: here y goes from 9.5 to 20.5.
    canvas::clear();
    stats::plotPoints({10.0, 20.0});
    img = canvas::snapshot();
    CHECK(isPen(img, 200 - 9, 100));   // y = 10, near the bottom
    CHECK(isPen(img, 9, 200));         // y = 20, near the top
    CHECK(!isPen(img, 100, 150));

    canvas::clear();
    canvas::setPenWidth(4);            // wide enough to cover whole pixels on the diagonal
    stats::plotLines({10.0, 20.0});
    img = canvas::snapshot();
    CHECK(isPen(img, 100, 150));       // half way along the line

    // All values the same: centred, not a zero-height scale.
    canvas::clear();
    stats::plotPoints({4.0, 4.0, 4.0});
    img = canvas::snapshot();
    CHECK(isPen(img, 100, 150));       // the middle point is in the middle of the canvas
}

}  // namespace

int main() {
    testNumbers();
    testInts();
    testPlots();
    return finish();
}
