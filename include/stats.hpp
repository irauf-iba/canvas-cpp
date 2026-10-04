// stats.hpp - statistics of a list of numbers, inspired by Princeton's
// StdStats.
//
//     #include <stats.hpp>
//
//     std::vector<double> heights = input::readAllDoubles();
//     std::cout << stats::mean(heights) << " " << stats::stddev(heights) << "\n";
//
// Each function takes a std::vector<double>, such as input::readAllDoubles()
// returns, or a list in braces: stats::mean({2, 4, 9}).
//
// Errors, such as the mean of an empty vector or a value that is NaN, print a
// message and stop the program.

#ifndef CANVAS_STATS_HPP
#define CANVAS_STATS_HPP

#include <vector>

namespace stats {

// The sum of the values; 0 for an empty vector.
double sum(const std::vector<double>& values);

// The smallest and the largest value.
double min(const std::vector<double>& values);
double max(const std::vector<double>& values);

// The average: the sum divided by the number of values.
double mean(const std::vector<double>& values);

// The middle value once the values are sorted, or the average of the two
// middle values when there is an even number of them. The vector itself is
// not reordered.
double median(const std::vector<double>& values);

// The sample variance and standard deviation, as in StdStats: the sum of the
// squared differences from the mean, divided by n - 1. They need at least two
// values.
double variance(const std::vector<double>& values);
double stddev(const std::vector<double>& values);

}  // namespace stats

#endif  // CANVAS_STATS_HPP
