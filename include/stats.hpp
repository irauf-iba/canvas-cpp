// stats.hpp - statistics of a list of numbers, inspired by Princeton's
// StdStats.
//
//     #include <stats.hpp>
//
//     std::vector<double> heights = input::readAllDoubles();
//     std::cout << stats::mean(heights) << " " << stats::stddev(heights) << "\n";
//     stats::plotBars(heights);                    // a bar chart on the canvas
//
// Each function takes a std::vector<double> or a std::vector<int>, such as the
// vectors input::readAllDoubles() and input::readAllInts() return, or a list
// in braces: stats::mean({2, 4, 9}).
//
// Errors, such as the mean of an empty vector or a value that is NaN, print a
// message and stop the program.

#ifndef CANVAS_STATS_HPP
#define CANVAS_STATS_HPP

#include <initializer_list>
#include <vector>

namespace stats {

// ---------------------------------------------------------------------------
// Totals and extremes
// ---------------------------------------------------------------------------

// The sum of the values; 0 for an empty vector. The sum of ints is a long
// long, so it doesn't overflow when the ints add up to more than an int holds.
double sum(const std::vector<double>& values);
long long sum(const std::vector<int>& values);

// The smallest and the largest value.
double min(const std::vector<double>& values);
int min(const std::vector<int>& values);
double max(const std::vector<double>& values);
int max(const std::vector<int>& values);

// ---------------------------------------------------------------------------
// Averages and spread
// ---------------------------------------------------------------------------

// The average: the sum divided by the number of values.
double mean(const std::vector<double>& values);
double mean(const std::vector<int>& values);

// The middle value once the values are sorted, or the average of the two
// middle values when there is an even number of them. The vector itself is
// not reordered.
double median(const std::vector<double>& values);
double median(const std::vector<int>& values);

// The sample variance and standard deviation, as in StdStats: the sum of the
// squared differences from the mean, divided by n - 1. They need at least two
// values.
double variance(const std::vector<double>& values);
double variance(const std::vector<int>& values);
double stddev(const std::vector<double>& values);
double stddev(const std::vector<int>& values);

// ---------------------------------------------------------------------------
// Plots
//
// Each draws values[i] at x = i on the canvas, with the current pen. It first
// sets the scale so the values fit: x from -1 to n, and y from a little below
// the smallest value to a little above the largest (including 0, for bars).
// Draw more on top in the same coordinates, or call setScale() again after.
// ---------------------------------------------------------------------------

// A dot for each value.
void plotPoints(const std::vector<double>& values);
void plotPoints(const std::vector<int>& values);

// A line through the values, from left to right.
void plotLines(const std::vector<double>& values);
void plotLines(const std::vector<int>& values);

// A bar from 0 up (or down) to each value.
void plotBars(const std::vector<double>& values);
void plotBars(const std::vector<int>& values);

// ---------------------------------------------------------------------------
// The same for a list in braces, such as stats::median({3, 1, 2}). (Without
// these, a list would fit both the double and the int versions.)
// ---------------------------------------------------------------------------

double sum(std::initializer_list<double> values);
double min(std::initializer_list<double> values);
double max(std::initializer_list<double> values);
double mean(std::initializer_list<double> values);
double median(std::initializer_list<double> values);
double variance(std::initializer_list<double> values);
double stddev(std::initializer_list<double> values);
void plotPoints(std::initializer_list<double> values);
void plotLines(std::initializer_list<double> values);
void plotBars(std::initializer_list<double> values);

}  // namespace stats

#endif  // CANVAS_STATS_HPP
