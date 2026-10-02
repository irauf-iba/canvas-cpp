// Tests for chance.hpp. A fixed seed makes every check deterministic, so the
// statistical checks can't fail by bad luck.
//
// Usage: chance_test         (main tests)
//        chance_test env     (run with CANVAS_SEED=12345)

#include <chance.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "check.hpp"

namespace {

void testSameNumbersEverywhere() {
    // These are the numbers for seed 42 on every computer and compiler; CI
    // checks them on Linux, Windows and macOS.
    chance::setSeed(42);
    CHECK(chance::seed() == 42);
    std::vector<int> rolls;
    for (int i = 0; i < 10; ++i) rolls.push_back(chance::uniform(1, 7));
    CHECK((rolls == std::vector<int>{1, 3, 5, 1, 6, 3, 5, 1, 5, 2}));
    CHECK(std::abs(chance::uniform() - 0.012382771132014692) < 1e-15);
    CHECK(std::abs(chance::uniform() - 0.52370558897433983) < 1e-15);
    CHECK(std::abs(chance::uniform() - 0.6852712867224986) < 1e-15);
    CHECK(std::abs(chance::gaussian(0, 1) - 0.4551532180080014) < 1e-12);
    CHECK((chance::permutation(10) == std::vector<int>{3, 1, 0, 7, 4, 8, 5, 9, 6, 2}));

    // Setting the seed again repeats the sequence.
    chance::setSeed(42);
    CHECK(chance::uniform(1, 7) == 1);
}

void testRanges() {
    chance::setSeed(1);
    std::vector<int> counts(6);
    const int n = 120000;
    bool inRange = true;
    for (int i = 0; i < n; ++i) {
        int r = chance::uniform(1, 7);
        if (r < 1 || r > 6) inRange = false;
        else ++counts[static_cast<std::size_t>(r - 1)];
    }
    CHECK(inRange);
    for (int c : counts) CHECK(std::abs(c - n / 6) < n / 100);  // each face about 1/6

    bool unitOk = true, intervalOk = true, nOk = true;
    for (int i = 0; i < 10000; ++i) {
        double u = chance::uniform();
        if (u < 0 || u >= 1) unitOk = false;
        double v = chance::uniform(-2.5, 4);  // int and double mixed
        if (v < -2.5 || v >= 4) intervalOk = false;
        int k = chance::uniform(3);
        if (k < 0 || k > 2) nOk = false;
    }
    CHECK(unitOk && intervalOk && nOk);
    CHECK(chance::uniform(-3, -2) == -3);  // a range with one value
}

void testDistributions() {
    chance::setSeed(7);
    const int n = 100000;
    double sum = 0, sumSquares = 0;
    int trues = 0, geometricSum = 0, poissonSum = 0;
    double exponentialSum = 0;
    for (int i = 0; i < n; ++i) {
        double g = chance::gaussian(10, 2);
        sum += g;
        sumSquares += g * g;
        if (chance::bernoulli(0.25)) ++trues;
        geometricSum += chance::geometric(0.2);
        poissonSum += chance::poisson(3.5);
        exponentialSum += chance::exponential(0.5);
    }
    double mean = sum / n, sd = std::sqrt(sumSquares / n - mean * mean);
    std::printf("gaussian(10, 2): mean %.3f, sd %.3f; bernoulli(0.25): %.4f; geometric(0.2) mean %.3f; "
                "poisson(3.5) mean %.3f; exponential(0.5) mean %.3f\n",
                mean, sd, static_cast<double>(trues) / n, static_cast<double>(geometricSum) / n,
                static_cast<double>(poissonSum) / n, exponentialSum / n);
    CHECK(std::abs(mean - 10) < 0.03 && std::abs(sd - 2) < 0.03);
    CHECK(std::abs(static_cast<double>(trues) / n - 0.25) < 0.005);
    CHECK(std::abs(static_cast<double>(geometricSum) / n - 5) < 0.05);  // 1/p
    CHECK(std::abs(static_cast<double>(poissonSum) / n - 3.5) < 0.03);
    CHECK(std::abs(exponentialSum / n - 2) < 0.03);                     // 1/rate

    std::vector<int> picked(3);
    for (int i = 0; i < n; ++i) ++picked[static_cast<std::size_t>(chance::discrete(std::vector<double>{0.5, 0.3, 0.2}))];
    CHECK(std::abs(picked[0] - n / 2) < n / 100 && std::abs(picked[2] - n / 5) < n / 100);
    bool zeroNeverPicked = true;
    for (int i = 0; i < 1000; ++i) {
        if (chance::discrete(std::vector<int>{1, 0, 3}) == 1) zeroNeverPicked = false;
    }
    CHECK(zeroNeverPicked);
    CHECK(chance::bernoulli(0) == false && chance::bernoulli(1) == true);
}

void testShuffle() {
    chance::setSeed(3);
    std::vector<std::string> words = {"a", "b", "c", "d", "e", "f"};
    std::vector<std::string> shuffled = words;
    chance::shuffle(shuffled);
    CHECK(shuffled != words);
    std::sort(shuffled.begin(), shuffled.end());
    CHECK(shuffled == words);  // same elements

    int numbers[5] = {1, 2, 3, 4, 5};
    chance::shuffle(numbers);
    std::sort(numbers, numbers + 5);
    CHECK(numbers[0] == 1 && numbers[4] == 5);

    std::vector<int> empty;
    chance::shuffle(empty);
    CHECK(chance::permutation(0).empty());
}

void testSeedFromEnvironment() {
    // Run with CANVAS_SEED=12345: the sequence is the one setSeed(12345) gives.
    CHECK(chance::seed() == 12345);
    std::vector<int> first;
    for (int i = 0; i < 5; ++i) first.push_back(chance::uniform(100));
    chance::setSeed(12345);
    std::vector<int> again;
    for (int i = 0; i < 5; ++i) again.push_back(chance::uniform(100));
    CHECK(first == again);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "env") {
        testSeedFromEnvironment();
        return finish();
    }
    testSameNumbersEverywhere();
    testRanges();
    testDistributions();
    testShuffle();
    return finish();
}
