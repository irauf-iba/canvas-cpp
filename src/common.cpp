// common.cpp - helpers shared by the draw, image and audio modules.

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "internal.hpp"

namespace draw_internal {

namespace {
bool gFailing = false;
}  // namespace

void fail(const char* module, const std::string& message) {
    std::fprintf(stderr, "%s: %s\n", module, message.c_str());
    gFailing = true;
    std::exit(1);
}

bool failing() { return gFailing; }

bool headlessRequested() {
    const char* value = std::getenv("DRAW_HEADLESS");
    return value && *value && std::strcmp(value, "0") != 0;
}

std::string lowerExtension(const std::string& filename) {
    std::size_t dot = filename.find_last_of('.');
    if (dot == std::string::npos) return "";
    std::string ext = filename.substr(dot + 1);
    for (char& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return ext;
}

}  // namespace draw_internal
