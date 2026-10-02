// text() with numbers and characters: how values are formatted, and that the
// overloads pick the right one for each kind of argument. Runs headless.

#include <canvas.hpp>

#include <cstdint>
#include <string>
#include <vector>

#include "check.hpp"

int main() {
    using canvas::detail::toText;
    CHECK(toText(42) == "42");
    CHECK(toText(-7L) == "-7");
    CHECK(toText(1000000.0) == "1000000");  // not 1e+06
    CHECK(toText(3.14159265) == "3.14159");
    CHECK(toText(0.1 + 0.2) == "0.3");
    CHECK(toText(2.5f) == "2.5");
    CHECK(toText(1e20) == "1e+20");
    CHECK(toText('A') == "A");              // a character, not 65
    CHECK(toText(std::uint8_t{200}) == "200");  // e.g. Color::r, a number
    CHECK(toText(true) == "true");
    CHECK(toText(std::vector<int>(3).size()) == "3");

    // Each call compiles and draws without error.
    std::vector<int> v(5);
    canvas::Color c = canvas::rgb(10, 20, 30);
    canvas::text(0.5, 0.9, "a string literal");
    canvas::text(0.5, 0.8, std::string("a std::string"));
    canvas::text(0.5, 0.7, 42);
    canvas::text(0.5, 0.6, 3.5);
    canvas::text(0.5, 0.5, 'x');
    canvas::text(0.5, 0.4, v.size());
    canvas::textLeft(0.1, 0.3, c.r);
    canvas::textRight(0.9, 0.2, 0);  // 0 is a number here, not a null string
    canvas::text(0.5, 0.1, "rotated", 30);
    return finish();
}
