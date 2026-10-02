// chance.cpp - implementation of chance.hpp.
//
// The generator is std::mt19937_64, whose output the C++ standard specifies
// exactly. The distributions are computed here rather than with the
// standard's distribution classes, whose results differ between compilers, so
// the same seed gives the same numbers everywhere.

#include "chance.hpp"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <random>
#include <string>

#include "internal.hpp"

namespace chance {
namespace {

struct State {
    bool seeded = false;
    std::uint64_t seed = 0;
    std::mt19937_64 engine;
};

State& st() {
    static State s;
    return s;
}

[[noreturn]] void fail(const std::string& message) { canvas_internal::fail("chance", message); }

void checkFinite(const char* function, double v) {
    if (!std::isfinite(v)) fail(std::string(function) + ": an argument is NaN or infinite");
}

// The engine, seeded on first use from CANVAS_SEED or else unpredictably.
std::mt19937_64& engine() {
    State& s = st();
    if (!s.seeded) {
        const char* text = std::getenv("CANVAS_SEED");
        if (text && *text) {
            char* end = nullptr;
            unsigned long long value = std::strtoull(text, &end, 10);
            if (*end != '\0') fail(std::string("CANVAS_SEED must be a whole number, not '") + text + "'");
            s.seed = value;
        } else {
            std::random_device device;
            auto now = static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
            s.seed = (static_cast<std::uint64_t>(device()) << 32) ^ device() ^ now;
        }
        s.engine.seed(s.seed);
        s.seeded = true;
    }
    return s.engine;
}

// 0 to n - 1 without bias, by rejecting the uneven top end of the range.
std::uint64_t below64(std::uint64_t n) {
    std::mt19937_64& e = engine();
    const std::uint64_t limit = UINT64_MAX - UINT64_MAX % n;
    std::uint64_t x;
    do {
        x = e();
    } while (x >= limit);
    return x % n;
}

// [0, 1) with 53 random bits.
double unit() { return static_cast<double>(engine()() >> 11) * 0x1.0p-53; }

}  // namespace

std::size_t detail::below(std::size_t n) { return static_cast<std::size_t>(below64(n)); }

void setSeed(std::uint64_t value) {
    State& s = st();
    s.seed = value;
    s.engine.seed(value);
    s.seeded = true;
}

std::uint64_t seed() {
    engine();
    return st().seed;
}

double uniform() { return unit(); }

int uniform(int n) {
    if (n <= 0) fail("uniform: n must be positive, not " + std::to_string(n));
    return static_cast<int>(below64(static_cast<std::uint64_t>(n)));
}

int uniform(int a, int b) {
    if (b <= a) {
        fail("uniform: the range " + std::to_string(a) + " to " + std::to_string(b) +
             " is empty (b must be greater than a)");
    }
    const auto span = static_cast<std::uint64_t>(static_cast<std::int64_t>(b) - a);
    return static_cast<int>(static_cast<std::int64_t>(a) + static_cast<std::int64_t>(below64(span)));
}

double uniform(double a, double b) {
    checkFinite("uniform", a);
    checkFinite("uniform", b);
    if (!(a < b)) fail("uniform: b must be greater than a");
    return a + unit() * (b - a);
}

double uniform(int a, double b) { return uniform(static_cast<double>(a), b); }
double uniform(double a, int b) { return uniform(a, static_cast<double>(b)); }

bool bernoulli(double p) {
    if (!(p >= 0 && p <= 1)) fail("bernoulli: p must be between 0 and 1");
    return unit() < p;
}

double gaussian() {
    // Marsaglia's polar method, as in StdRandom.
    double r, x, y;
    do {
        x = uniform(-1.0, 1.0);
        y = uniform(-1.0, 1.0);
        r = x * x + y * y;
    } while (r >= 1 || r == 0);
    return x * std::sqrt(-2 * std::log(r) / r);
}

double gaussian(double mean, double stddev) {
    checkFinite("gaussian", mean);
    checkFinite("gaussian", stddev);
    if (stddev < 0) fail("gaussian: stddev must not be negative");
    return mean + stddev * gaussian();
}

int geometric(double p) {
    if (!(p > 0 && p <= 1)) fail("geometric: p must be greater than 0 and at most 1");
    if (p == 1) return 1;
    return static_cast<int>(std::ceil(std::log(1 - unit()) / std::log(1 - p)));
}

int poisson(double mean) {
    if (!(mean > 0) || !std::isfinite(mean)) fail("poisson: the mean must be positive");
    if (mean > 500) fail("poisson: the mean must be at most 500");
    // Knuth's method, as in StdRandom.
    const double limit = std::exp(-mean);
    int k = 0;
    double product = 1;
    do {
        ++k;
        product *= unit();
    } while (product >= limit);
    return k - 1;
}

double exponential(double rate) {
    if (!(rate > 0) || !std::isfinite(rate)) fail("exponential: the rate must be positive");
    return -std::log(1 - unit()) / rate;
}

int discrete(const std::vector<double>& probabilities) {
    if (probabilities.empty()) fail("discrete: the list of probabilities is empty");
    double sum = 0;
    for (double p : probabilities) {
        if (!(p >= 0) || !std::isfinite(p)) fail("discrete: probabilities must not be negative");
        sum += p;
    }
    if (std::abs(sum - 1) > 1e-9) fail("discrete: probabilities must add up to 1, not " + std::to_string(sum));
    for (;;) {  // repeat in the rare case rounding leaves r just above the total
        double r = unit(), total = 0;
        for (std::size_t i = 0; i < probabilities.size(); ++i) {
            total += probabilities[i];
            if (r < total) return static_cast<int>(i);
        }
    }
}

int discrete(const std::vector<int>& frequencies) {
    if (frequencies.empty()) fail("discrete: the list of frequencies is empty");
    std::int64_t sum = 0;
    for (int f : frequencies) {
        if (f < 0) fail("discrete: frequencies must not be negative");
        sum += f;
    }
    if (sum == 0) fail("discrete: at least one frequency must be positive");
    auto r = static_cast<std::int64_t>(below64(static_cast<std::uint64_t>(sum)));
    std::int64_t total = 0;
    for (std::size_t i = 0; i < frequencies.size(); ++i) {
        total += frequencies[i];
        if (r < total) return static_cast<int>(i);
    }
    return static_cast<int>(frequencies.size()) - 1;  // not reached
}

std::vector<int> permutation(int n) {
    if (n < 0) fail("permutation: n must not be negative");
    std::vector<int> p(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) p[static_cast<std::size_t>(i)] = i;
    shuffle(p);
    return p;
}

}  // namespace chance
