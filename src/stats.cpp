// stats.cpp - implementation of stats.hpp.

#include "stats.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>

#include "internal.hpp"

namespace stats {
namespace {

[[noreturn]] void fail(const char* function, const std::string& message) {
    canvas_internal::fail("stats", std::string(function) + ": " + message);
}

void checkNotEmpty(const char* function, const std::vector<double>& values) {
    if (values.empty()) fail(function, "the vector is empty");
}

// NaN is always a mistake (0.0 / 0, or reading a value that went wrong), and
// it would make comparisons, and so min, max and median, meaningless.
void checkNoNaN(const char* function, const std::vector<double>& values) {
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (std::isnan(values[i])) fail(function, "values[" + std::to_string(i) + "] is NaN");
    }
}

double meanOf(const char* function, const std::vector<double>& values) {
    checkNotEmpty(function, values);
    checkNoNaN(function, values);
    double total = 0;
    for (double v : values) total += v;
    return total / static_cast<double>(values.size());
}

double varianceOf(const char* function, const std::vector<double>& values) {
    if (values.size() < 2) {
        fail(function, "needs at least 2 values, not " + std::to_string(values.size()));
    }
    // Two passes, subtracting the mean first: accurate even when the values
    // are large and close together.
    double average = meanOf(function, values);
    double squares = 0;
    for (double v : values) squares += (v - average) * (v - average);
    return squares / static_cast<double>(values.size() - 1);
}

}  // namespace

double sum(const std::vector<double>& values) {
    checkNoNaN("sum", values);
    double total = 0;
    for (double v : values) total += v;
    return total;
}

double min(const std::vector<double>& values) {
    checkNotEmpty("min", values);
    checkNoNaN("min", values);
    return *std::min_element(values.begin(), values.end());
}

double max(const std::vector<double>& values) {
    checkNotEmpty("max", values);
    checkNoNaN("max", values);
    return *std::max_element(values.begin(), values.end());
}

double mean(const std::vector<double>& values) { return meanOf("mean", values); }

double median(const std::vector<double>& values) {
    checkNotEmpty("median", values);
    checkNoNaN("median", values);
    std::vector<double> sorted = values;  // a copy: the caller's order stays
    std::sort(sorted.begin(), sorted.end());
    std::size_t middle = sorted.size() / 2;
    if (sorted.size() % 2 == 1) return sorted[middle];
    return (sorted[middle - 1] + sorted[middle]) / 2;
}

double variance(const std::vector<double>& values) { return varianceOf("variance", values); }
double stddev(const std::vector<double>& values) { return std::sqrt(varianceOf("stddev", values)); }

}  // namespace stats
