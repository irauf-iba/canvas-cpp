# chance: random numbers

`#include <chance.hpp>`

The `chance` module is a C++ version of Princeton's
[StdRandom](https://introcs.cs.princeton.edu/java/stdlib/javadoc/StdRandom.html).
It gives random numbers without the setup that C++'s `<random>` needs.

```cpp
#include <chance.hpp>
#include <iostream>

int main() {
    int roll = chance::uniform(1, 7);   // 1 to 6
    std::cout << "You rolled " << roll << "\n";
    if (chance::bernoulli(0.25)) std::cout << "Lucky!\n";   // 25% of the time
}
```

The header [`include/chance.hpp`](../include/chance.hpp) documents every
function. (The name `random` isn't possible: it is already a function in
`<cstdlib>`.)

## Ranges

Ranges include the lower end and exclude the upper end, as in StdRandom:
`uniform(1, 7)` gives 1, 2, 3, 4, 5 or 6, and `uniform(n)` gives 0 to n − 1,
which suits indexing a vector: `words[chance::uniform(words.size())]`.

## Functions

| Function | Gives |
|---|---|
| `uniform()` | A real number from 0 to 1 (1 excluded). |
| `uniform(n)` | An integer from 0 to n − 1. |
| `uniform(a, b)` | An integer from a to b − 1, or a real number from a to b if either is a real number, e.g. `uniform(0, 2.5)`. |
| `bernoulli(p)` | `true` with probability p (default 0.5). |
| `gaussian()`, `gaussian(mean, stddev)` | A number from the normal distribution (default mean 0, standard deviation 1). |
| `geometric(p)` | The number of tries until the first success, when each try succeeds with probability p. |
| `poisson(mean)` | An integer from the Poisson distribution. |
| `exponential(rate)` | A number from the exponential distribution. |
| `discrete(probabilities)` | An index i, chosen with probability `probabilities[i]`; they must add up to 1. |
| `discrete(frequencies)` | An index i, chosen in proportion to `frequencies[i]` (integers). |
| `shuffle(v)` | Puts a `std::vector` or an array in a random order. |
| `permutation(n)` | The numbers 0 to n − 1 in a random order. |

## Repeating a run

Each run gives different numbers. To get the same numbers every run, for
example to track down a bug, set the seed first:

```cpp
chance::setSeed(42);
```

`chance::seed()` returns the current seed. Printing it at the start makes any
interesting run repeatable. Setting the environment variable `CANVAS_SEED`
does the same as `setSeed` without changing the program, which is useful for
automated grading:

```sh
CANVAS_SEED=42 ./student_program
```

With the same seed, the numbers are the same on every computer and compiler,
and CI checks this on Linux, Windows and macOS. (C++'s own distribution
classes don't promise this: `std::normal_distribution` gives different
numbers with different compilers.)

## Errors

Mistakes stop the program with a message and exit status 1, for example:

```
chance: uniform: the range 5 to 5 is empty (b must be greater than a)
chance: uniform: n must be positive, not 0
chance: bernoulli: p must be between 0 and 1
chance: discrete: probabilities must add up to 1, not 0.900000
```

## Differences from Princeton's StdRandom

| StdRandom | chance | Why |
|---|---|---|
| `StdRandom.uniformInt(a, b)`, `uniformDouble(a, b)` | `chance::uniform(a, b)` | One name; integer or real depends on the arguments. |
| `shuffle(Object[] a)` and friends | `shuffle(v)` for any vector or array | One template for every element type. |
| `pareto`, `cauchy`, `sample` | Not included | Rarely used in a first course. |
| Exceptions | Message and exit | Clearer for beginners than an uncaught exception. |

## How it works

- **Generator.** The generator is `std::mt19937_64`, the 64-bit Mersenne
  Twister, whose output the C++ standard specifies exactly.
- **Distributions.** These are computed in the library, with the same methods
  as StdRandom, rather than with the standard's distribution classes.
  Integers come out without bias, by rejecting the uneven top of the range.
- **Without a seed,** the generator is seeded from `std::random_device` mixed
  with the clock.

## Example

[`examples/random_walk.cpp`](../examples/random_walk.cpp) draws a random walk
on a grid and prints its seed, so a walk can be repeated with
`CANVAS_SEED=<seed> ./random_walk`.
