// chance.hpp - random numbers, inspired by Princeton's StdRandom.
//
//     #include <chance.hpp>
//
//     int roll = chance::uniform(1, 7);           // 1 to 6
//     if (chance::bernoulli(0.25)) { ... }        // true 25% of the time
//     double height = chance::gaussian(170, 10);  // mean 170, std dev 10
//     chance::shuffle(cards);
//
// Ranges include the lower end and exclude the upper end, as in StdRandom:
// uniform(1, 7) gives 1, 2, 3, 4, 5 or 6.
//
// Each run gives different numbers. To get the same numbers every run, for
// example while debugging, call setSeed() first, or set the environment
// variable CANVAS_SEED to a number (useful for automated grading). With the
// same seed, the numbers are the same on every computer and compiler.
//
// Errors, such as an empty range, print a message and stop the program.

#ifndef CANVAS_CHANCE_HPP
#define CANVAS_CHANCE_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

namespace chance {

// ---------------------------------------------------------------------------
// Seed
// ---------------------------------------------------------------------------

// Starts the sequence of random numbers from the given seed.
void setSeed(std::uint64_t seed);

// The seed of the current sequence. Print it to be able to repeat a run.
std::uint64_t seed();

// ---------------------------------------------------------------------------
// Uniform
// ---------------------------------------------------------------------------

// A real number from 0 (included) to 1 (excluded).
double uniform();

// An integer from 0 to n - 1.
int uniform(int n);

// An integer from a to b - 1.
int uniform(int a, int b);

// A real number from a (included) to b (excluded).
double uniform(double a, double b);
double uniform(int a, double b);
double uniform(double a, int b);

// ---------------------------------------------------------------------------
// Other distributions
// ---------------------------------------------------------------------------

// True with probability p (default 0.5), false otherwise.
bool bernoulli(double p = 0.5);

// A real number from the normal (Gaussian) distribution with the given mean
// and standard deviation (default 0 and 1).
double gaussian();
double gaussian(double mean, double stddev);

// The number of tries until the first success, when each try succeeds with
// probability p: 1, 2, 3, ...
int geometric(double p);

// An integer from the Poisson distribution with the given mean.
int poisson(double mean);

// A real number from the exponential distribution with the given rate.
double exponential(double rate);

// An index i, chosen with probability probabilities[i]. The probabilities
// must not be negative and must add up to 1.
int discrete(const std::vector<double>& probabilities);

// An index i, chosen with probability frequencies[i] / (sum of frequencies).
int discrete(const std::vector<int>& frequencies);

// ---------------------------------------------------------------------------
// Shuffling
// ---------------------------------------------------------------------------

// Puts the elements in a random order. Works with a std::vector or an array.
template <typename T>
void shuffle(std::vector<T>& v);
template <typename T, std::size_t N>
void shuffle(T (&a)[N]);

// The numbers 0 to n - 1 in a random order.
std::vector<int> permutation(int n);

// ---------------------------------------------------------------------------
// Implementation of the templates
// ---------------------------------------------------------------------------

namespace detail {
std::size_t below(std::size_t n);  // 0 to n - 1, for any size
}

template <typename T>
void shuffle(std::vector<T>& v) {
    for (std::size_t i = v.size(); i > 1; --i) {
        std::size_t j = detail::below(i);
        T temporary = v[i - 1];
        v[i - 1] = v[j];
        v[j] = temporary;
    }
}

template <typename T, std::size_t N>
void shuffle(T (&a)[N]) {
    for (std::size_t i = N; i > 1; --i) {
        std::size_t j = detail::below(i);
        T temporary = a[i - 1];
        a[i - 1] = a[j];
        a[j] = temporary;
    }
}

}  // namespace chance

#endif  // CANVAS_CHANCE_HPP
