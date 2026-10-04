// Tests for stats.hpp.

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
    CHECK(near(stats::sum({}), 0));                     // an empty sum is 0
    CHECK(near(stats::min({-0.5, -3, 1e300}), -3));
    CHECK(near(stats::mean({1, 2, 6}), 3));             // whole numbers in braces work too
    CHECK(near(stats::median({1, 2, 3, 4}), 2.5));

    // median sorts a copy, not the caller's vector.
    std::vector<double> unsorted = {9, 1, 5};
    stats::median(unsorted);
    CHECK((unsorted == std::vector<double>{9, 1, 5}));

    // Large values close together: two passes keep the variance exact.
    CHECK(near(stats::variance({1e9 + 4, 1e9 + 7, 1e9 + 13, 1e9 + 16}), 30));
}

void testFromInts() {
    // A vector of ints, e.g. from input::readAllInts(), converts in one line.
    std::vector<int> rolls = {3, 1, 4, 1, 5, 9};
    std::vector<double> values(rolls.begin(), rolls.end());
    CHECK(near(stats::mean(values), 23.0 / 6));
    CHECK(near(stats::median(values), 3.5));
}

}  // namespace

int main() {
    testNumbers();
    testFromInts();
    return finish();
}
