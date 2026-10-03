// Tests for input.hpp that need no keyboard: fromFile() with plain std::cin,
// and the helpers on in-memory streams. Standard input is tested by
// stdin_test and ask_test.

#include <input.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "check.hpp"

namespace {

void write(const std::string& name, const std::string& bytes) {
    std::ofstream(name, std::ios::binary) << bytes;
}

void testFromFileWithPlainCin() {
    // A file saved on Windows with a byte-order mark.
    write("reading_records.txt", "\xEF\xBB\xBF" "3 people\r\nAli Khan\r\n\r\n\r\n10 20\r\n  30\r\n");
    input::fromFile("reading_records.txt");
    int n = 0;
    std::cin >> n;                        // ordinary std::cin reads the file
    CHECK(n == 3);
    input::skipRestOfLine();              // " people"
    std::string name;
    std::getline(std::cin, name);         // plain std::getline: no '\r' left
    CHECK(name == "Ali Khan");
    input::skipEmptyLines();
    CHECK((input::readAllInts() == std::vector<int>{10, 20, 30}));
    CHECK(!(std::cin >> n));              // the end of the file
}

void testHelpersOnStreams() {
    std::istringstream lines("first\r\n\r\nthird\nlast");
    CHECK((input::readAllLines(lines) == std::vector<std::string>{"first", "", "third", "last"}));

    std::istringstream words("  to be\tor\n\nnot  ");
    CHECK((input::readAllWords(words) == std::vector<std::string>{"to", "be", "or", "not"}));

    std::istringstream doubles("1 2.5\n-3e2 +4");
    CHECK((input::readAllDoubles(doubles) == std::vector<double>{1, 2.5, -300, 4}));

    std::istringstream bom("\xEF\xBB\xBF" "7 8");
    CHECK((input::readAllInts(bom) == std::vector<int>{7, 8}));

    std::istringstream blank("  \n \r\n");
    CHECK(input::readAllInts(blank).empty());

    std::istringstream records("\n\n\r\nx\n\ny");
    std::string line;
    input::skipEmptyLines(records);
    input::getLine(records, line);
    CHECK(line == "x");
    input::skipEmptyLines(records);
    input::getLine(records, line);
    CHECK(line == "y");

    std::istringstream crlf("a\r\nb\r\n");
    std::vector<std::string> got;
    while (input::getLine(crlf, line)) got.push_back(line);
    CHECK((got == std::vector<std::string>{"a", "b"}));

    std::istringstream rest("25 years old\nAli\n");
    int age = 0;
    rest >> age;
    input::skipRestOfLine(rest);
    input::getLine(rest, line);
    CHECK(age == 25 && line == "Ali");
}

void testStringHelpers() {
    CHECK((input::split("  to be  or ") == std::vector<std::string>{"to", "be", "or"}));
    CHECK(input::split("").empty());
    CHECK((input::split("Ali,23,,B+", ',') == std::vector<std::string>{"Ali", "23", "", "B+"}));
    CHECK((input::split("a,", ',') == std::vector<std::string>{"a", ""}));
    CHECK(input::trim("  hello world \t\r\n") == "hello world");
    CHECK(input::trim("   ").empty());
    CHECK(input::toInt(" 42 ") == 42);
    CHECK(input::toInt("-7") == -7 && input::toInt("+3") == 3);
    CHECK(input::toInt("2147483647") == 2147483647);
    CHECK(input::toDouble("2.5") == 2.5 && input::toDouble("1e6") == 1e6 && input::toDouble("3") == 3);
}

}  // namespace

int main() {
    testFromFileWithPlainCin();
    testHelpersOnStreams();
    testStringHelpers();
    return finish();
}
