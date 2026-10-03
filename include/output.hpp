// output.hpp - sending standard output to a file.
//
// Programs write with std::cout (or printf), and the shell can send that to
// a file:
//
//     ./average < scores.txt > result.txt
//
// In an IDE, where redirecting output is awkward, call output::toFile() once
// at the start of main() instead.

#ifndef CANVAS_OUTPUT_HPP
#define CANVAS_OUTPUT_HPP

#include <string>

namespace output {

// From now on, everything written to std::cout or with printf goes to this
// file instead of the screen. The file is created, or emptied if it exists.
// Calling it again switches to another file.
void toFile(const std::string& filename);

}  // namespace output

#endif  // CANVAS_OUTPUT_HPP
