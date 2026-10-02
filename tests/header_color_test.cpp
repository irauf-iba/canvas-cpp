// color.hpp compiles on its own (part of header_test).

#include <color.hpp>

static_assert(canvas::rgb(1, 2, 3) == image::rgb(1, 2, 3), "canvas and image share Color");
