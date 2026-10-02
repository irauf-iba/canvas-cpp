// color.hpp compiles on its own (part of header_test).

#include <color.hpp>

static_assert(draw::rgb(1, 2, 3) == image::rgb(1, 2, 3), "draw and image share Color");
