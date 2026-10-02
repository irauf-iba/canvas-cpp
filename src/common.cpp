// common.cpp - helpers shared by the canvas, image and audio modules.

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <SDL3/SDL.h>

#include "internal.hpp"

namespace canvas_internal {

namespace {
bool gFailing = false;
bool gShuttingDown = false;
}  // namespace

void beginShutdown() { gShuttingDown = true; }

bool shuttingDown() { return gShuttingDown; }

void fail(const char* module, const std::string& message) {
    std::fprintf(stderr, "%s: %s\n", module, message.c_str());
    gFailing = true;
    std::exit(1);
}

bool failing() { return gFailing; }

bool headlessRequested() {
    const char* value = std::getenv("CANVAS_HEADLESS");
    return value && *value && std::strcmp(value, "0") != 0;
}

namespace {

bool canOpen(const std::string& path) {
    SDL_IOStream* io = SDL_IOFromFile(path.c_str(), "rb");  // handles UTF-8 names on Windows
    if (!io) return false;
    SDL_CloseIO(io);
    return true;
}

bool isAbsolute(const std::string& path) {
    return !path.empty() && (path[0] == '/' || path[0] == '\\' || (path.size() > 1 && path[1] == ':'));
}

}  // namespace

std::string findInputFile(const std::string& filename) {
    if (canOpen(filename)) return filename;
    const char* programFolder = SDL_GetBasePath();  // ends with a separator
    if (programFolder && !isAbsolute(filename)) {
        std::string beside = std::string(programFolder) + filename;
        if (canOpen(beside)) return beside;
    }
    return "";
}

std::string notFound(const std::string& filename) {
    std::string message = "cannot open '" + filename + "'";
    if (isAbsolute(filename)) return message + " (no such file)";
    char* current = SDL_GetCurrentDirectory();
    const char* programFolder = SDL_GetBasePath();
    message += " (not in the current folder";
    if (current) message += ", " + std::string(current);
    if (programFolder && (!current || std::string(current) != programFolder)) {
        message += ", or the program's folder, " + std::string(programFolder);
    }
    SDL_free(current);
    return message + ")";
}

std::string lowerExtension(const std::string& filename) {
    std::size_t dot = filename.find_last_of('.');
    if (dot == std::string::npos) return "";
    std::string ext = filename.substr(dot + 1);
    for (char& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return ext;
}

}  // namespace canvas_internal
