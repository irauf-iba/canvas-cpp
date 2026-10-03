// Reads real standard input, which tests/run_with_input.cmake connects to a
// file with Windows line endings, using std::cin with the input helpers.
//
// Usage: stdin_test < tests/data/stdin_input.txt

#include <input.hpp>

#include <iostream>
#include <string>
#include <vector>

int main() {
    int age = 0;
    std::cin >> age;
    input::skipRestOfLine();               // " years" and the line end
    std::string name;
    input::getLine(std::cin, name);        // no '\r', even on Linux
    std::vector<int> numbers = input::readAllInts();
    long sum = 0;
    for (int x : numbers) sum += x;
    std::cout << "age=" << age << " name=[" << name << "] count=" << numbers.size() << " sum=" << sum << "\n";
}
