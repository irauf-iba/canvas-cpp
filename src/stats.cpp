// stats.cpp - implementation of stats.hpp.

#include "stats.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>

#include "canvas.hpp"
#include "internal.hpp"

namespace stats {
namespace {

[[noreturn]] void fail(const char* function, const std::string& message) {
    canvas_internal::fail("stats", std::string(function) + ": " + message);
}

void checkNotEmpty(const char* function, std::size_t size) {
    if (size == 0) fail(function, "the vector is empty");
}

// NaN is always a mistake (0.0 / 0, or reading a value that went wrong), and
// it would make comparisons, and so min, max and median, meaningless.
void checkNoNaN(const char* function, const std::vector<double>& values) {
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (std::isnan(values[i])) fail(function, "values[" + std::to_string(i) + "] is NaN");
    }
}

std::vector<double> toDoubles(const std::vector<int>& values) {
    return std::vector<double>(values.begin(), values.end());
}

double meanOf(const char* function, const std::vector<double>& values) {
    checkNotEmpty(function, values.size());
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

// --- Plots ----------------------------------------------------------------------

// Sets the scale for plotting the values: x from -1 to n, y around the values
// (and 0, for bars) with a margin.
void fitScale(const char* function, const std::vector<double>& values, bool includeZero) {
    checkNotEmpty(function, values.size());
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (!std::isfinite(values[i])) {
            fail(function, "values[" + std::to_string(i) + "] is " + (std::isnan(values[i]) ? "NaN" : "infinite"));
        }
    }
    double lo = *std::min_element(values.begin(), values.end());
    double hi = *std::max_element(values.begin(), values.end());
    if (includeZero) {
        lo = std::min(lo, 0.0);
        hi = std::max(hi, 0.0);
    }
    double span = hi - lo;
    if (span == 0) {  // all values the same: centre them
        span = hi != 0 ? std::abs(hi) : 1;
        lo -= span / 2;
        hi += span / 2;
    } else {
        lo -= span * 0.05;
        hi += span * 0.05;
    }
    canvas::setXscale(-1, static_cast<double>(values.size()));
    canvas::setYscale(lo, hi);
}

void points(const std::vector<double>& values) {
    fitScale("plotPoints", values, false);
    for (std::size_t i = 0; i < values.size(); ++i) {
        double x = static_cast<double>(i);
        // A sixth of the spacing, so neighbours stay apart, and at least the
        // pen's dot, so they don't vanish when there are many values.
        canvas::filledCircle(x, values[i], 1.0 / 6);
        canvas::point(x, values[i]);
    }
}

void lines(const std::vector<double>& values) {
    fitScale("plotLines", values, false);
    if (values.size() == 1) {
        canvas::point(0, values[0]);
        return;
    }
    std::vector<canvas::Point> path;
    for (std::size_t i = 0; i < values.size(); ++i) path.push_back({static_cast<double>(i), values[i]});
    canvas::polyline(path);
}

void bars(const std::vector<double>& values) {
    fitScale("plotBars", values, true);
    for (std::size_t i = 0; i < values.size(); ++i) {
        canvas::filledRectangle(static_cast<double>(i), values[i] / 2, 0.25, std::abs(values[i]) / 2);
    }
}

}  // namespace

// --- Totals and extremes ---------------------------------------------------------

double sum(const std::vector<double>& values) {
    checkNoNaN("sum", values);
    double total = 0;
    for (double v : values) total += v;
    return total;
}

long long sum(const std::vector<int>& values) {
    long long total = 0;
    for (int v : values) total += v;
    return total;
}

double min(const std::vector<double>& values) {
    checkNotEmpty("min", values.size());
    checkNoNaN("min", values);
    return *std::min_element(values.begin(), values.end());
}

int min(const std::vector<int>& values) {
    checkNotEmpty("min", values.size());
    return *std::min_element(values.begin(), values.end());
}

double max(const std::vector<double>& values) {
    checkNotEmpty("max", values.size());
    checkNoNaN("max", values);
    return *std::max_element(values.begin(), values.end());
}

int max(const std::vector<int>& values) {
    checkNotEmpty("max", values.size());
    return *std::max_element(values.begin(), values.end());
}

// --- Averages and spread -----------------------------------------------------------

double mean(const std::vector<double>& values) { return meanOf("mean", values); }
double mean(const std::vector<int>& values) { return meanOf("mean", toDoubles(values)); }

double median(const std::vector<double>& values) {
    checkNotEmpty("median", values.size());
    checkNoNaN("median", values);
    std::vector<double> sorted = values;  // a copy: the caller's order stays
    std::sort(sorted.begin(), sorted.end());
    std::size_t middle = sorted.size() / 2;
    if (sorted.size() % 2 == 1) return sorted[middle];
    return (sorted[middle - 1] + sorted[middle]) / 2;
}

double median(const std::vector<int>& values) { return median(toDoubles(values)); }

double variance(const std::vector<double>& values) { return varianceOf("variance", values); }
double variance(const std::vector<int>& values) { return varianceOf("variance", toDoubles(values)); }
double stddev(const std::vector<double>& values) { return std::sqrt(varianceOf("stddev", values)); }
double stddev(const std::vector<int>& values) { return std::sqrt(varianceOf("stddev", toDoubles(values))); }

// --- Plots -------------------------------------------------------------------------

void plotPoints(const std::vector<double>& values) { points(values); }
void plotPoints(const std::vector<int>& values) { points(toDoubles(values)); }
void plotLines(const std::vector<double>& values) { lines(values); }
void plotLines(const std::vector<int>& values) { lines(toDoubles(values)); }
void plotBars(const std::vector<double>& values) { bars(values); }
void plotBars(const std::vector<int>& values) { bars(toDoubles(values)); }

// --- Lists in braces -----------------------------------------------------------------

double sum(std::initializer_list<double> values) { return sum(std::vector<double>(values)); }
double min(std::initializer_list<double> values) { return min(std::vector<double>(values)); }
double max(std::initializer_list<double> values) { return max(std::vector<double>(values)); }
double mean(std::initializer_list<double> values) { return mean(std::vector<double>(values)); }
double median(std::initializer_list<double> values) { return median(std::vector<double>(values)); }
double variance(std::initializer_list<double> values) { return variance(std::vector<double>(values)); }
double stddev(std::initializer_list<double> values) { return stddev(std::vector<double>(values)); }
void plotPoints(std::initializer_list<double> values) { points(std::vector<double>(values)); }
void plotLines(std::initializer_list<double> values) { lines(std::vector<double>(values)); }
void plotBars(std::initializer_list<double> values) { bars(std::vector<double>(values)); }

}  // namespace stats
