// Minimal test helpers: CHECK records a failure and continues.

#ifndef CANVAS_TESTS_CHECK_HPP
#define CANVAS_TESTS_CHECK_HPP

#include <cstdio>

inline int& failureCount() {
    static int count = 0;
    return count;
}

inline void check(bool ok, const char* expression, const char* file, int line) {
    if (ok) return;
    std::printf("%s:%d: FAILED: %s\n", file, line, expression);
    ++failureCount();
}

#define CHECK(condition) check((condition), #condition, __FILE__, __LINE__)

// Prints a summary and returns the exit code for main().
inline int finish() {
    if (failureCount() == 0) {
        std::printf("all checks passed\n");
        return 0;
    }
    std::printf("%d check(s) failed\n", failureCount());
    return 1;
}

#endif  // CANVAS_TESTS_CHECK_HPP
