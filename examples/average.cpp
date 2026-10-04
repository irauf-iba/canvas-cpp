// Reads numbers from standard input and prints how many there are, their
// average, median, standard deviation, smallest and largest:
//
//     ./average < examples/data/numbers.txt
//
// Or type numbers and end with Ctrl+D (Ctrl+Z then Enter on Windows).

#include <input.hpp>
#include <stats.hpp>

#include <iostream>
#include <vector>

int main() {
    // Like while (std::cin >> x), but a value that isn't a number stops the
    // program with a message saying where it is.
    std::vector<double> numbers = input::readAllDoubles();
    if (numbers.empty()) {
        std::cout << "no numbers\n";
        return 0;
    }
    std::cout << numbers.size() << " numbers\n";
    std::cout << "average   " << stats::mean(numbers) << "\n";
    std::cout << "median    " << stats::median(numbers) << "\n";
    if (numbers.size() > 1) {  // the standard deviation needs two numbers
        std::cout << "std dev   " << stats::stddev(numbers) << "\n";
    }
    std::cout << "smallest  " << stats::min(numbers) << "\n";
    std::cout << "largest   " << stats::max(numbers) << "\n";
}
