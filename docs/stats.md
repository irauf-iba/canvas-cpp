# stats: statistics of a list of numbers

`#include <stats.hpp>`

The `stats` module is a C++ version of Princeton's
[StdStats](https://introcs.cs.princeton.edu/java/stdlib/javadoc/StdStats.html):
the average, median, spread and extremes of a list of numbers.

```cpp
#include <input.hpp>
#include <stats.hpp>
#include <iostream>

int main() {
    std::vector<double> scores = input::readAllDoubles();   // ./scores < scores.txt
    std::cout << "average " << stats::mean(scores) << "\n";
    std::cout << "median  " << stats::median(scores) << "\n";
    std::cout << "spread  " << stats::stddev(scores) << "\n";
}
```

The header [`include/stats.hpp`](../include/stats.hpp) documents every
function.

## Functions

Every function takes a `std::vector<double>`, such as
`input::readAllDoubles()` returns, or a list in braces:
`stats::mean({2.5, 4, 9})`.

| Function | Gives |
|---|---|
| `sum(v)` | The total; 0 for an empty vector. |
| `min(v)`, `max(v)` | The smallest and the largest value. |
| `mean(v)` | The average: the sum divided by the number of values. |
| `median(v)` | The middle value once the values are sorted, or the average of the two middle values. `v` itself is not reordered. |
| `variance(v)`, `stddev(v)` | The sample variance and standard deviation, dividing by n − 1 as StdStats does. They need at least two values. |

For whole numbers, such as the vector `input::readAllInts()` returns, make a
vector of doubles from them first:

```cpp
std::vector<int> rolls = input::readAllInts();
std::vector<double> values(rolls.begin(), rolls.end());
std::cout << stats::mean(values) << "\n";
```

## Plotting

The module doesn't draw. A plot of the values is a few lines with
[canvas](canvas.md), and a good exercise: value i at x = i, with the scale
set to fit.

```cpp
canvas::setXscale(-1, v.size());
canvas::setYscale(0, stats::max(v) * 1.1);
for (std::size_t i = 0; i < v.size(); ++i) {
    canvas::filledRectangle(i, v[i] / 2, 0.25, v[i] / 2);   // a bar chart
}
```

## Errors

Mistakes stop the program with a message and exit status 1, for example:

```
stats: mean: the vector is empty
stats: stddev: needs at least 2 values, not 1
stats: median: values[1] is NaN
```

NaN ("not a number", from `0.0 / 0` or `std::sqrt(-1)`) is always an error,
because it makes comparisons, and so `min`, `max` and `median`, meaningless.
An empty vector is an error everywhere except `sum`, whose total is 0.

## Differences from Princeton's StdStats

| StdStats | stats | Why |
|---|---|---|
| `StdStats.var(a)`, `stddev(a)` | `stats::variance(v)`, `stddev(v)` | The full word is clearer. |
| `varp`, `stddevp` (population variance) | Not included | One choice is enough in a first course; the sample versions are the usual ones. |
| Arrays of `double` and `int` | A vector of `double`, or a list in braces | Vectors are what C++ students use, and one type keeps the module small. |
| `plotPoints`, `plotLines`, `plotBars` | Not included | Plotting with `canvas` is short, and worth writing. |
| `IllegalArgumentException` | Message and exit | Clearer for beginners than an uncaught exception. |

## Examples

- [`examples/average.cpp`](../examples/average.cpp): the average, median,
  standard deviation, smallest and largest of numbers on standard input:
  `./average < examples/data/numbers.txt`.
- [`examples/bernoulli.cpp`](../examples/bernoulli.cpp):
  coin-flip experiments, with their mean and standard deviation next to the
  curve's.
