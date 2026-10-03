// The ask... functions, with answers from tests/data/ask_input.txt on
// standard input (via tests/run_with_input.cmake). The prompts and messages
// are captured with output::toFile and compared with what a user would see.
// The result is reported on stderr, since stdout goes to the file.

#include <input.hpp>
#include <output.hpp>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int main() {
    output::toFile("ask_transcript.txt");
    int age = input::askInt("Age? ");                  // "abc", "", " 42"
    int choice = input::askInt("Choice? ", 1, 4);      // "9", "3"
    double height = input::askDouble("Height? ");      // "x", "1.75"
    bool again = input::askYesNo("Again? ");           // "maybe", "YES"
    std::string name = input::askLine("Name? ");       // "Ali Khan"
    int n = 0;
    std::cin >> n;                                      // "7 extra"
    input::skipRestOfLine();
    std::string next = input::askLine("Next name? ");  // "Sara"
    std::cout.flush();
    output::toFile("ask_transcript_done.txt");         // finishes the transcript

    std::ifstream in("ask_transcript.txt");
    std::stringstream transcript;
    transcript << in.rdbuf();
    const std::string expected =
        "Age? Please enter a whole number.\nAge? "
        "Choice? Please enter a whole number from 1 to 4.\nChoice? "
        "Height? Please enter a number.\nHeight? "
        "Again? Please answer y or n.\nAgain? "
        "Name? Next name? ";
    bool ok = age == 42 && choice == 3 && height == 1.75 && again && name == "Ali Khan" && n == 7 &&
              next == "Sara" && transcript.str() == expected;
    std::fprintf(stderr, "age=%d choice=%d height=%g again=%d name=[%s] n=%d next=[%s]\ntranscript=[%s]\n%s\n",
                 age, choice, height, again, name.c_str(), n, next.c_str(), transcript.str().c_str(),
                 ok ? "PASSED" : "FAILED");
    return ok ? 0 : 1;
}
