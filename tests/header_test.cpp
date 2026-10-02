// Compiled with strict warnings as errors: the public header must be clean,
// self-contained and safe to include twice.

#include <canvas.hpp>
#include <canvas.hpp>

static_assert(canvas::rgb(300, -5, 128).r == 255, "rgb() clamps high values");
static_assert(canvas::rgb(300, -5, 128).g == 0, "rgb() clamps low values");
static_assert(canvas::rgb(300, -5, 128).b == 128, "rgb() keeps values in range");
static_assert(canvas::rgb(1, 2, 3).a == 255, "rgb() is opaque by default");
static_assert(canvas::Color{1, 2, 3}.a == 255, "Color is opaque by default");

int main() {}
