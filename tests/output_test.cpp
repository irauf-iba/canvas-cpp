// output::toFile sends std::cout and printf to a file, and calling it again
// switches files. Results are reported on stderr, since stdout is redirected.

#include <output.hpp>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int main() {
    output::toFile("output_test_1.txt");
    std::cout << "hello " << 42 << "\n";
    std::printf("printf %d\n", 7);
    output::toFile("output_test_2.txt");  // switches, and finishes the first file
    std::cout << "second\n";
    std::cout.flush();

    std::ifstream in("output_test_1.txt");
    std::stringstream contents;
    contents << in.rdbuf();
    bool ok = contents.str() == "hello 42\nprintf 7\n";
    std::fprintf(stderr, "output_test_1.txt holds \"%s\": %s\n", contents.str().c_str(), ok ? "PASSED" : "FAILED");
    return ok ? 0 : 1;
}
