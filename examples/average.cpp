// Reads numbers from standard input and prints how many there are, their
// average, smallest and largest:
//
//     ./average < examples/data/numbers.txt
//
// Or type numbers and end with Ctrl+D (Ctrl+Z then Enter on Windows).

#include <input.hpp>

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
    double count = static_cast<double>(numbers.size());
    double sum = 0, smallest = numbers[0], largest = numbers[0];
    for (double x : numbers) {
        sum += x;
        if (x < smallest) smallest = x;
        if (x > largest) largest = x;
    }
    std::cout << numbers.size() << " numbers, average " << sum / count << ", smallest "
              << smallest << ", largest " << largest << "\n";
}
