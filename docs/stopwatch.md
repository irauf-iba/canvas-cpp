# stopwatch: timing code

`#include <stopwatch.hpp>`

The `stopwatch` module is a C++ version of Princeton's
[Stopwatch](https://introcs.cs.princeton.edu/java/stdlib/javadoc/Stopwatch.html),
for measuring how long code takes, e.g. in lessons on the running time of
algorithms.

```cpp
#include <stopwatch.hpp>
#include <iostream>

int main() {
    stopwatch::start();
    long long sum = 0;
    for (int i = 0; i < 100000000; ++i) sum += i % 7;
    std::cout << sum << " in " << stopwatch::elapsed() << " seconds\n";
}
```

## Functions

| Function | Does |
|---|---|
| `stopwatch::start()` | Starts timing from now; calling it again starts again from 0. |
| `stopwatch::elapsed()` | The seconds since `start()`, or since the program started if `start()` wasn't called. Reading it doesn't stop the stopwatch. |

There is one stopwatch, like there is one canvas. For several measurements,
read `elapsed()` into variables:

```cpp
stopwatch::start();
sortWithSelection(a);
double selection = stopwatch::elapsed();

stopwatch::start();
sortWithMerge(b);
double merge = stopwatch::elapsed();
```

## Measuring well

- **Use an optimized (Release) build.** A Debug build can be ten times
  slower, and by different amounts for different code, so comparisons are
  unreliable.
- **Time something long enough.** Below about a millisecond, the result is
  mostly noise; repeat the work in a loop and divide.
- **The doubling test.** To see how the time grows, time the same code for n,
  2n, 4n and so on. If the time doubles each time, the code is linear; if it
  goes up four times, quadratic.
- **Wall-clock time.** `elapsed()` measures real time, so other programs
  running at the same time can slow a measurement. It uses a steady clock,
  which doesn't jump when the computer's clock is changed.

## Differences from Princeton's Stopwatch

| Stopwatch | stopwatch | Why |
|---|---|---|
| `Stopwatch timer = new Stopwatch()` | `stopwatch::start()` | No objects; one stopwatch. |
| `timer.elapsedTime()` | `stopwatch::elapsed()` | Shorter. |
