// Reads lines of "name,score" and prints each student's grade:
//
//     ./grades < examples/data/scores.csv
//     ./grades < examples/data/scores.csv > grades.txt
//
// The first line holds the column names and is skipped.

#include <input.hpp>

#include <iostream>
#include <string>
#include <vector>

std::string grade(int score) {
    if (score >= 85) return "A";
    if (score >= 70) return "B";
    if (score >= 55) return "C";
    return "F";
}

int main() {
    // In an IDE, read the file directly instead of redirecting:
    // input::fromFile("scores.csv");
    std::string line;
    input::getLine(std::cin, line);  // the header: name,score
    while (input::getLine(std::cin, line)) {
        if (input::trim(line).empty()) continue;  // skip blank lines
        std::vector<std::string> fields = input::split(line, ',');
        std::string name = input::trim(fields[0]);
        int score = input::toInt(fields[1]);
        std::cout << name << ": " << score << " (" << grade(score) << ")\n";
    }
}
