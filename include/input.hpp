// input.hpp - helpers for reading input with std::cin.
//
// Programs read with std::cin as usual; these functions help where std::cin
// trips beginners up:
//
//     int age;
//     std::cin >> age;
//     input::skipRestOfLine();               // or getline reads an empty line
//     std::string name;
//     input::getLine(std::cin, name);        // like std::getline, without '\r'
//
//     int choice = input::askInt("Choice (1-3)? ", 1, 3);   // asks until valid
//
//     std::vector<int> scores = input::readAllInts();        // the rest of std::cin
//
// As in Princeton's course, programs can read a file through standard input
// (./average < scores.txt). In an IDE, call input::fromFile("scores.txt") at
// the start of main() instead: std::cin then reads the file.
//
// Mistakes stop the program with a clear message, for example
//     input: readAllInts: line 3: 'abc' is not a whole number

#ifndef CANVAS_INPUT_HPP
#define CANVAS_INPUT_HPP

#include <iostream>
#include <string>
#include <vector>

namespace input {

// ---------------------------------------------------------------------------
// Working with std::cin
// ---------------------------------------------------------------------------

// Throws away the rest of the current line, up to and including its '\n'.
// Call it after std::cin >> x and before reading a line, which would
// otherwise read only the empty end of the line x was on. It never waits for
// more input, so it is safe right before a prompt.
void skipRestOfLine(std::istream& in = std::cin);

// Skips lines that are completely empty, up to the next line with something
// on it (a line of only spaces counts as not empty). For data files with
// blank lines between records. On the keyboard it waits for the next line,
// so don't call it right before a prompt.
void skipEmptyLines(std::istream& in = std::cin);

// Reads one line into `line`, like std::getline, but without the '\r' that a
// file saved on Windows leaves at the end of every line on Linux and macOS.
// Use it the same way:  while (input::getLine(std::cin, line)) { ... }
std::istream& getLine(std::istream& in, std::string& line);

// ---------------------------------------------------------------------------
// Reading everything that is left
//
// Like  while (std::cin >> x) v.push_back(x);  except that a value of the
// wrong kind stops the program with a message instead of silently ending the
// loop. They read std::cin unless another stream is given.
// ---------------------------------------------------------------------------

std::vector<int> readAllInts(std::istream& in = std::cin);
std::vector<double> readAllDoubles(std::istream& in = std::cin);
std::vector<std::string> readAllWords(std::istream& in = std::cin);

// All remaining lines, including empty ones, without '\r'.
std::vector<std::string> readAllLines(std::istream& in = std::cin);

// ---------------------------------------------------------------------------
// Asking questions
//
// Each prints the prompt, reads a whole line as the answer, and asks again
// until the answer is valid ("Please enter a whole number."). Empty lines are
// ignored (except by askLine). If the input ends, the program stops with a
// message instead of asking forever.
// ---------------------------------------------------------------------------

int askInt(const std::string& prompt);
int askInt(const std::string& prompt, int min, int max);
double askDouble(const std::string& prompt);
double askDouble(const std::string& prompt, double min, double max);

// The answer as typed, which may be empty. After std::cin >> x, call
// skipRestOfLine() first.
std::string askLine(const std::string& prompt);

// true for y or yes, false for n or no, in any case.
bool askYesNo(const std::string& prompt);

// ---------------------------------------------------------------------------
// Reading a file instead of the keyboard
// ---------------------------------------------------------------------------

// From now on, std::cin reads this file, so a program written for
// ./program < scores.txt works unchanged in an IDE. Windows line endings read
// as '\n' and a byte-order mark at the start is skipped. Like other files the
// library reads, it is looked up in the current folder and then next to the
// program.
void fromFile(const std::string& filename);

// ---------------------------------------------------------------------------
// Taking strings apart
// ---------------------------------------------------------------------------

// The words of s, split at spaces, tabs and new lines:
//     split("  to be  or ")  gives  {"to", "be", "or"}
std::vector<std::string> split(const std::string& s);

// The parts of s between each delimiter, including empty ones:
//     split("Ali,23,,B+", ',')  gives  {"Ali", "23", "", "B+"}
std::vector<std::string> split(const std::string& s, char delimiter);

// s without spaces, tabs and new lines at either end.
std::string trim(const std::string& s);

// The number in s (spaces around it are allowed). Stops with a clear message
// if s is not a number, e.g. toInt("3.5") or toDouble("abc").
int toInt(const std::string& s);
double toDouble(const std::string& s);

}  // namespace input

#endif  // CANVAS_INPUT_HPP
