// window.cpp - the window, events and frames: opening the window, showing the
// canvas, input, show() and pause(), pausing with the debug keys, recording
// frames, and keeping the window open after main() returns.

#include <SDL3/SDL.h>
// Only for SDL_SetMainReady(): the student writes an ordinary main().
#define SDL_MAIN_HANDLED
#define SDL_MAIN_NOIMPL
#include <SDL3/SDL_main.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "canvas_impl.hpp"

namespace canvas::impl {

// ---------------------------------------------------------------------------
// Window, presentation and events
// ---------------------------------------------------------------------------

void onExit();

void sizeWindow() {
    State& s = st();
    double displayScale = 1;
    float density = 1;
    if (s.window) {
        float ds = SDL_GetWindowDisplayScale(s.window);
        float pd = SDL_GetWindowPixelDensity(s.window);
        if (ds > 0) displayScale = ds;
        if (pd > 0) density = pd;
    }
    s.pw = std::max(1, static_cast<int>(std::lround(s.width * displayScale)));
    s.ph = std::max(1, static_cast<int>(std::lround(s.height * displayScale)));
    s.scale = static_cast<double>(s.pw) / s.width;
    s.windowW = static_cast<float>(s.pw) / density;
    s.windowH = static_cast<float>(s.ph) / density;
    if (s.window) {
        SDL_SetWindowSize(s.window, static_cast<int>(std::lround(s.windowW)),
                          static_cast<int>(std::lround(s.windowH)));
        SDL_SyncWindow(s.window);
    }
}

void createCanvas() {
    State& s = st();
    s.pixels.assign(static_cast<std::size_t>(s.pw) * static_cast<std::size_t>(s.ph) * 4, 255);
    s.dirty = true;
    if (!s.renderer) return;
    if (s.texture) SDL_DestroyTexture(s.texture);
    s.texture = SDL_CreateTexture(s.renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
                                  s.pw, s.ph);
    if (!s.texture) fail(std::string("cannot create the canvas texture: ") + SDL_GetError());
    SDL_SetTextureBlendMode(s.texture, SDL_BLENDMODE_NONE);
    SDL_SetTextureScaleMode(s.texture, SDL_SCALEMODE_LINEAR);
}

void openWindow() {
    State& s = st();
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");  // let Ctrl+C stop the program
    SDL_SetMainReady();
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fail(std::string("cannot open a window: ") + SDL_GetError() +
             "\n      (set CANVAS_HEADLESS=1 to draw without a window)");
    }
    s.window = SDL_CreateWindow(s.title.c_str(), s.width, s.height,
                                SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!s.window) fail(std::string("cannot open a window: ") + SDL_GetError());
    sizeWindow();
    s.renderer = SDL_CreateRenderer(s.window, nullptr);
    if (!s.renderer) fail(std::string("cannot create a renderer: ") + SDL_GetError());
    SDL_SetRenderVSync(s.renderer, s.doubleBuffered ? 1 : 0);
    SDL_ShowWindow(s.window);
    SDL_StartTextInput(s.window);
    SDL_GetMouseState(&s.mouseWX, &s.mouseWY);
}

void ensureInit() {
    State& s = st();
    if (s.initialized) return;
    s.initialized = true;
    s.headless = canvas_internal::headlessRequested();
    if (const char* h = std::getenv("CANVAS_HINTS"); h && std::strcmp(h, "0") == 0) s.hints = false;
    if (s.headless) {
        sizeWindow();
    } else {
        openWindow();
    }
    createCanvas();
    present();
    std::atexit(onExit);
}

// Shows the texture as it is, e.g. after the window was uncovered.
void repaint() {
    State& s = st();
    SDL_RenderClear(s.renderer);
    SDL_RenderTexture(s.renderer, s.texture, nullptr, nullptr);
    SDL_RenderPresent(s.renderer);
}

// Sets the window title, followed by the mouse position and the frame rate
// if showMouseCoordinates() is on.
void updateTitle() {
    State& s = st();
    if (!s.window) return;
    s.lastTitleUpdate = SDL_GetTicksNS();
    s.titleStale = false;
    const std::string paused = s.debugPaused ? " | paused: P resumes, N steps" : "";
    if (!s.showCoordinates) {
        SDL_SetWindowTitle(s.window, (s.title + paused).c_str());
        return;
    }
    double x = s.xmin + s.mouseWX / s.windowW * (s.xmax - s.xmin);
    double y = s.ymax - s.mouseWY / s.windowH * (s.ymax - s.ymin);
    // About three significant digits for the range shown: 0.534 for 0 to 1, 53.4 for 0 to 100.
    auto decimals = [](double range) {
        return std::clamp(3 - static_cast<int>(std::floor(std::log10(std::abs(range)))), 0, 6);
    };
    // The frame rate only while frames are being shown, e.g. in an animation.
    char rate[32] = "";
    if (s.fps > 0 && s.lastTitleUpdate - s.lastPresent < 1'500'000'000) {
        std::snprintf(rate, sizeof rate, " | %.0f fps", s.fps);
    }
    char buffer[512];
    std::snprintf(buffer, sizeof buffer, "%s | x %.*f, y %.*f%s%s", s.title.c_str(), decimals(s.xmax - s.xmin), x,
                  decimals(s.ymax - s.ymin), y, rate, paused.c_str());
    SDL_SetWindowTitle(s.window, buffer);
}

// Copies the canvas to the screen.
void present() {
    State& s = st();
    s.dirty = false;
    if (s.headless) return;
    const std::uint8_t* shown = hasOverlay() ? withOverlay().data() : s.pixels.data();
    SDL_UpdateTexture(s.texture, nullptr, shown, s.pw * 4);
    repaint();
    s.lastPresent = SDL_GetTicksNS();
    ++s.framesCounted;
    if (s.lastPresent - s.framesSince >= 1'000'000'000) {
        s.fps = s.framesCounted * 1e9 / static_cast<double>(s.lastPresent - s.framesSince);
        s.framesCounted = 0;
        s.framesSince = s.lastPresent;
        if (s.showCoordinates) updateTitle();
    }
}

// Keys already read free their slots, so the limit applies to unread keys.
void pushKey(char c) {
    State& s = st();
    if (s.keyCount == kKeyBufferSize && s.keyRead > 0) {
        std::copy(s.keys.begin() + s.keyRead, s.keys.end(), s.keys.begin());
        s.keyCount -= s.keyRead;
        s.keyRead = 0;
    }
    if (s.keyCount < kKeyBufferSize) s.keys[static_cast<std::size_t>(s.keyCount++)] = c;
}

// The Key for an SDL keycode, or -1 if Key has no such key.
int keyIndexOf(SDL_Keycode k) {
    if (k >= SDLK_A && k <= SDLK_Z) return static_cast<int>(Key::A) + static_cast<int>(k - SDLK_A);
    if (k >= SDLK_0 && k <= SDLK_9) return static_cast<int>(Key::Num0) + static_cast<int>(k - SDLK_0);
    switch (k) {
    case SDLK_SPACE: return static_cast<int>(Key::Space);
    case SDLK_RETURN:
    case SDLK_KP_ENTER: return static_cast<int>(Key::Enter);
    case SDLK_ESCAPE: return static_cast<int>(Key::Escape);
    case SDLK_BACKSPACE: return static_cast<int>(Key::Backspace);
    case SDLK_TAB: return static_cast<int>(Key::Tab);
    case SDLK_LEFT: return static_cast<int>(Key::Left);
    case SDLK_RIGHT: return static_cast<int>(Key::Right);
    case SDLK_UP: return static_cast<int>(Key::Up);
    case SDLK_DOWN: return static_cast<int>(Key::Down);
    case SDLK_LSHIFT:
    case SDLK_RSHIFT: return static_cast<int>(Key::Shift);
    case SDLK_LCTRL:
    case SDLK_RCTRL: return static_cast<int>(Key::Control);
    case SDLK_LALT:
    case SDLK_RALT: return static_cast<int>(Key::Alt);
    default: return -1;
    }
}

bool isDebugKey(SDL_Keycode k) { return k == SDLK_P || k == SDLK_N || k == SDLK_G; }

// P, N and G with debug keys on. They are not passed on to the program.
void handleDebugKey(SDL_Keycode k) {
    State& s = st();
    if (k == SDLK_P) {
        s.debugPaused = !s.debugPaused;
        s.debugStep = false;
    } else if (k == SDLK_N) {
        if (s.debugPaused) s.debugStep = true;
        else s.debugPaused = true;  // pause at the next frame, then step from there
    } else {
        toggleGrid();
    }
    updateTitle();
}

void handleEvent(const SDL_Event& e) {
    State& s = st();
    switch (e.type) {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        s.exiting = true;
        std::exit(0);
    case SDL_EVENT_WINDOW_EXPOSED:
        repaint();
        break;
    case SDL_EVENT_MOUSE_MOTION:
        s.mouseWX = e.motion.x;
        s.mouseWY = e.motion.y;
        s.titleStale = s.showCoordinates;
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        s.mouseWX = e.button.x;
        s.mouseWY = e.button.y;
        s.clicked = true;
        break;
    case SDL_EVENT_TEXT_INPUT:
        for (const char* p = e.text.text; *p; ++p) {
            if (s.debugKeys && std::strchr("pPnNgG", *p)) continue;  // a debug key, not typing
            if (*p >= 32 && *p < 127) pushKey(*p);  // ASCII only; UTF-8 bytes are >= 128
        }
        break;
    case SDL_EVENT_KEY_DOWN:
        if (s.debugKeys && isDebugKey(e.key.key)) {
            if (!e.key.repeat) handleDebugKey(e.key.key);
            break;
        }
        if (!e.key.repeat) {
            int k = keyIndexOf(e.key.key);
            if (k >= 0) s.keyWentDown[static_cast<std::size_t>(k)] = true;
        }
        // Keys that type a character but produce no text input event.
        switch (e.key.key) {
        case SDLK_RETURN:
        case SDLK_KP_ENTER: pushKey('\n'); break;
        case SDLK_BACKSPACE: pushKey('\b'); break;
        case SDLK_TAB: pushKey('\t'); break;
        case SDLK_ESCAPE: pushKey(27); break;
        case SDLK_DELETE: pushKey(127); break;
        default: break;
        }
        break;
    default:
        break;
    }
}

// Handles queued events. Asking the OS for new events is rate-limited, so
// that input queries stay cheap when called very often (e.g. once per audio
// sample); events already in SDL's queue are always handled.
void pollEvents() {
    State& s = st();
    if (s.headless) return;
    Uint64 now = SDL_GetTicksNS();
    if (now - s.lastPump >= kInputPumpInterval) {
        SDL_PumpEvents();
        s.lastPump = now;
    }
    SDL_Event e;
    while (SDL_PeepEvents(&e, 1, SDL_GETEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST) > 0) handleEvent(e);
    if (s.titleStale && now - s.lastTitleUpdate > 30'000'000) updateTitle();
}

// Lets the OS know the window is alive, but leaves input events queued for
// the next query, show() or pause(). Ends the program if the window was closed.
void pumpIfDue(Uint64 now) {
    State& s = st();
    if (now - s.lastPump < kPumpInterval) return;
    SDL_PumpEvents();
    s.lastPump = now;
    // While the program is ending (e.g. audio finishing its sound), leave the
    // event for onExit(): calling exit() from an exit handler is not allowed.
    if (SDL_HasEvent(SDL_EVENT_QUIT) && !canvas_internal::shuttingDown()) {
        s.exiting = true;
        std::exit(0);
    }
}

// Waits ms milliseconds, keeping the window responsive.
void waitWithEvents(int ms) {
    const Uint64 end = SDL_GetTicksNS() + static_cast<Uint64>(ms) * 1'000'000;
    for (Uint64 now = SDL_GetTicksNS(); now < end; now = SDL_GetTicksNS()) {
        pollEvents();
        SDL_DelayPrecise(std::min<Uint64>(end - now, 10'000'000));
    }
}

// With debug keys, after P: keeps showing the frame until P (resume) or N
// (one more frame). Other input is ignored while paused.
void waitWhilePaused() {
    State& s = st();
    if (!s.debugPaused || s.headless || !s.window) return;
    if (s.debugStep) {  // stepping: this is the next frame, so stop again here
        s.debugStep = false;
    }
    updateTitle();
    present();
    while (s.debugPaused && !s.debugStep) {
        SDL_Event e;
        if (!SDL_WaitEventTimeout(&e, 100)) continue;
        switch (e.type) {
        case SDL_EVENT_QUIT:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        case SDL_EVENT_WINDOW_EXPOSED:
        case SDL_EVENT_MOUSE_MOTION:
            handleEvent(e);
            if (s.titleStale) updateTitle();  // the coordinates readout keeps working
            break;
        case SDL_EVENT_KEY_DOWN:
            if (isDebugKey(e.key.key) && !e.key.repeat) {
                handleDebugKey(e.key.key);
                present();  // e.g. the grid turned on or off
            }
            break;
        default:
            break;
        }
    }
    // Start the timing afresh, as after a long pause: no catching up.
    s.hasFrame = false;
    s.hasPaused = false;
    s.recordTicks = SDL_GetTicksNS();
    s.lastPump = 0;
}

// At each frame (show(), pause(), or a drawing call in slow motion), records
// the canvas if a recording is running. seconds is how long the frame will
// be shown, or < 0 if that isn't known (then the time until the next frame
// is measured).
void recordFrame(double seconds) {
    if (!canvas_internal::recordingActive()) return;
    State& s = st();
    const Uint64 now = SDL_GetTicksNS();
    if (s.recordStarted) {
        // A frame without a known length: as long as it was on screen, or
        // one sixtieth of a second when nothing is shown.
        double previous = s.recordNext;
        if (previous < 0) previous = s.headless ? 1.0 / 60 : static_cast<double>(now - s.recordTicks) / 1e9;
        s.recordClock += previous;
    }
    s.recordStarted = true;
    s.recordNext = seconds;
    s.recordTicks = now;
    canvas_internal::recordingFrame(canvasImage(), s.recordClock);
}

// Ends a recording with the canvas as it is now.
void finishRecording(bool canFail) {
    if (!canvas_internal::recordingActive()) return;
    State& s = st();
    double end = s.recordClock;
    if (s.recordStarted) {
        end += s.recordNext >= 0 ? s.recordNext
                                 : (s.headless ? 1.0 / 60 : static_cast<double>(SDL_GetTicksNS() - s.recordTicks) / 1e9);
    }
    canvas_internal::recordingFinish(canvasImage(), end, canFail);
}

// Called after every drawing operation.
void afterDraw() {
    State& s = st();
    if (s.checkVisible && !s.changedPixel && !s.drewOffscreen) {
        hint(kHintSameColour, std::string(s.drawFunction) + " at (" + num(s.drawX) + ", " + num(s.drawY) +
                                  ") can't be seen: it is drawn in " + colourName(s.pen) + " on a " +
                                  colourName(s.background) + " background. Change the colour with setPenColor().");
    }
    s.checkVisible = false;
    const bool wasDirty = s.dirty;
    s.dirty = true;
    if (s.headless) {
        if (s.drawDelay > 0) recordFrame(s.drawDelay / 1000.0);
        return;
    }
    Uint64 now = SDL_GetTicksNS();
    if (!wasDirty) s.dirtySince = now;
    if (s.drawDelay > 0) {  // slow motion: show every step
        present();
        recordFrame(s.drawDelay / 1000.0);
        waitWithEvents(s.drawDelay);
        waitWhilePaused();
        return;
    }
    if (s.doubleBuffered && now - s.dirtySince > 2'000'000'000) {
        hint(kHintNoShow, "enableDoubleBuffering() is on, so drawing appears on screen only when show() is called.");
    }
    if (!s.doubleBuffered && now - s.lastPresent >= kPresentInterval) present();
    pumpIfDue(now);
}

// Entry point for drawing functions.
void beginDraw() {
    ensureInit();
    st().atFrameBoundary = false;
}

// Entry point for a drawing call at (x, y), recorded for hints. usesPen is
// false for pictures, which don't draw in the pen colour.
void beginDraw(const char* function, double x, double y, bool usesPen) {
    beginDraw();
    State& s = st();
    s.drawFunction = function;
    s.drawX = x;
    s.drawY = y;
    s.drewOffscreen = false;
    s.changedPixel = false;
    s.checkVisible = usesPen && s.hints && !(s.hintsShown & kHintSameColour) && s.pen == s.background &&
                     s.pen.a > 0;
    if (usesPen && s.pen.a == 0) {
        hint(kHintTransparent, "the pen colour is fully transparent (alpha 0), so " + std::string(function) +
                                   " draws nothing. Use an alpha above 0, or leave it out.");
    }
}

void beginDraw(const char* function, const std::vector<Point>& points) {
    beginDraw(function, points.empty() ? 0 : points[0].x, points.empty() ? 0 : points[0].y);
}

// Entry point for input queries: show pending drawing, then read events.
void beginInput() {
    beginDraw();
    State& s = st();
    if (s.headless) return;
    if (!s.doubleBuffered && s.dirty && SDL_GetTicksNS() - s.lastPresent >= kPresentInterval) {
        present();
    }
    pollEvents();
}

// show() and pause() end a frame. Input from that frame is discarded, unless
// the previous call was also show() or pause() (e.g. show() then pause()), in
// which case the events read in between belong to the next frame.
void endFrame() {
    ensureInit();
    State& s = st();
    if (s.atFrameBoundary) return;
    s.atFrameBoundary = true;
    s.clicked = false;
    s.keyCount = 0;
    s.keyRead = 0;
    s.keyWentDown.fill(false);
}

void onExit() {
    canvas_internal::beginShutdown();
    State& s = st();
    if (!canvas_internal::failing()) finishRecording(false);
    if (!s.window) return;
    if (!s.exiting && !canvas_internal::failing()) {
        // Keep the final picture on screen until the user closes the window.
        present();
        SDL_Event e;
        while (SDL_WaitEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT || e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) break;
            if (e.type == SDL_EVENT_WINDOW_EXPOSED) repaint();
            if (e.type == SDL_EVENT_MOUSE_MOTION && s.showCoordinates) {  // keep the readout going
                s.mouseWX = e.motion.x;
                s.mouseWY = e.motion.y;
                updateTitle();
            }
        }
    }
    // Only video: the audio module may still be finishing its sound.
    SDL_DestroyWindow(s.window);
    s.window = nullptr;
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

// Maps a Key to the SDL keycode of the key with that label.
SDL_Keycode keycodeOf(Key key) {
    auto offset = [key](Key first) { return static_cast<SDL_Keycode>(static_cast<int>(key) - static_cast<int>(first)); };
    if (key >= Key::A && key <= Key::Z) return SDLK_A + offset(Key::A);
    if (key >= Key::Num0 && key <= Key::Num9) return SDLK_0 + offset(Key::Num0);
    switch (key) {
    case Key::Space: return SDLK_SPACE;
    case Key::Enter: return SDLK_RETURN;
    case Key::Escape: return SDLK_ESCAPE;
    case Key::Backspace: return SDLK_BACKSPACE;
    case Key::Tab: return SDLK_TAB;
    case Key::Left: return SDLK_LEFT;
    case Key::Right: return SDLK_RIGHT;
    case Key::Up: return SDLK_UP;
    case Key::Down: return SDLK_DOWN;
    case Key::Shift: return SDLK_LSHIFT;
    case Key::Control: return SDLK_LCTRL;
    case Key::Alt: return SDLK_LALT;
    default: return SDLK_UNKNOWN;
    }
}

}  // namespace canvas::impl

namespace canvas {

using namespace impl;

void enableDoubleBuffering() {
    State& s = st();
    s.doubleBuffered = true;
    if (s.renderer) SDL_SetRenderVSync(s.renderer, 1);
}

void disableDoubleBuffering() {
    State& s = st();
    s.doubleBuffered = false;
    if (s.renderer) SDL_SetRenderVSync(s.renderer, 0);
    if (s.initialized) present();
}

void setFrameRate(double framesPerSecond) {
    checkFinite("setFrameRate", {framesPerSecond});
    checkNonNegative("setFrameRate", "the frame rate", framesPerSecond);
    State& s = st();
    s.frameInterval = framesPerSecond > 0 ? static_cast<Uint64>(std::llround(1e9 / framesPerSecond)) : 0;
    s.hasFrame = false;
}

void show() {
    endFrame();
    State& s = st();
    if (s.frameInterval > 0 && !s.headless) {
        // Wait until one frame interval after the previous frame was due, so
        // frames are evenly spaced. A frame that is late is shown at once,
        // and the schedule restarts from it rather than trying to catch up.
        const Uint64 start = SDL_GetTicksNS();
        const Uint64 due = s.lastFrame + s.frameInterval;
        if (s.hasFrame && due > start) {
            Uint64 now = start;
            while (now < due) {
                pollEvents();
                SDL_DelayPrecise(std::min<Uint64>(due - now, 10'000'000));
                now = SDL_GetTicksNS();
            }
            s.lastFrame = due;
        } else {
            s.lastFrame = start;
        }
        s.hasFrame = true;
    }
    present();
    pollEvents();
    recordFrame(s.frameInterval > 0 ? static_cast<double>(s.frameInterval) / 1e9 : -1);
    waitWhilePaused();
}

void pause(int ms) {
    if (ms < 0) fail("pause: ms must not be negative");
    endFrame();
    State& s = st();
    recordFrame(ms / 1000.0);
    if (s.headless) return;
    if (!s.doubleBuffered && s.dirty) present();

    // When pause() is called repeatedly, wait relative to the end of the
    // previous pause so the time spent drawing counts toward this one. After
    // a long gap, wait the full time from now.
    const Uint64 length = static_cast<Uint64>(ms) * 1'000'000;
    const Uint64 start = SDL_GetTicksNS();
    Uint64 target = start + length;
    if (s.hasPaused && start - s.pauseEnd < 2 * length) target = s.pauseEnd + length;

    Uint64 now = start;
    while (true) {
        pollEvents();
        now = SDL_GetTicksNS();
        if (now >= target) break;
        SDL_DelayPrecise(std::min<Uint64>(target - now, 10'000'000));
    }
    s.hasPaused = true;
    s.pauseEnd = target > start ? target : now;
    waitWhilePaused();
}

void startRecording(const std::string& filename) {
    ensureInit();
    canvas_internal::recordingStart(filename);
    State& s = st();
    s.recordStarted = false;
    s.recordClock = 0;
    s.recordNext = -1;
}

void stopRecording() {
    ensureInit();
    finishRecording(true);
}

// --- Mouse -------------------------------------------------------------------

double mouseX() {
    beginInput();
    const State& s = st();
    return s.xmin + s.mouseWX / s.windowW * (s.xmax - s.xmin);
}

double mouseY() {
    beginInput();
    const State& s = st();
    return s.ymax - s.mouseWY / s.windowH * (s.ymax - s.ymin);
}

bool isMousePressed() {
    beginInput();
    if (st().headless) return false;
    return SDL_GetMouseState(nullptr, nullptr) != 0;
}

bool mouseClicked() {
    beginInput();
    State& s = st();
    bool clicked = s.clicked;
    s.clicked = false;
    return clicked;
}

// --- Keyboard ----------------------------------------------------------------

bool hasNextKeyTyped() {
    beginInput();
    const State& s = st();
    return s.keyRead < s.keyCount;
}

char nextKeyTyped() {
    beginInput();
    State& s = st();
    if (s.keyRead >= s.keyCount) fail("nextKeyTyped: no key was typed (check hasNextKeyTyped() first)");
    return s.keys[static_cast<std::size_t>(s.keyRead++)];
}

bool wasKeyPressed(Key key) {
    beginInput();
    bool& pressed = st().keyWentDown[static_cast<std::size_t>(key)];
    bool result = pressed;
    pressed = false;
    return result;
}

bool isKeyPressed(Key key) {
    beginInput();
    if (st().headless) return false;
    if (st().debugKeys && (key == Key::P || key == Key::N || key == Key::G)) return false;
    const bool* state = SDL_GetKeyboardState(nullptr);
    auto down = [&](SDL_Keycode k) { return state[SDL_GetScancodeFromKey(k, nullptr)]; };
    switch (key) {
    case Key::Shift: return down(SDLK_LSHIFT) || down(SDLK_RSHIFT);
    case Key::Control: return down(SDLK_LCTRL) || down(SDLK_RCTRL);
    case Key::Alt: return down(SDLK_LALT) || down(SDLK_RALT);
    case Key::Enter: return down(SDLK_RETURN) || down(SDLK_KP_ENTER);
    default: return down(keycodeOf(key));
    }
}

bool isMouseOver(double x, double y, double halfWidth, double halfHeight) {
    checkFinite("isMouseOver", {x, y, halfWidth, halfHeight});
    checkNonNegative("isMouseOver", "halfWidth", halfWidth);
    checkNonNegative("isMouseOver", "halfHeight", halfHeight);
    return std::abs(mouseX() - x) <= halfWidth && std::abs(mouseY() - y) <= halfHeight;
}

}  // namespace canvas

void canvas_internal::keepWindowAlive() {
    if (canvas::impl::st().window) canvas::impl::pumpIfDue(SDL_GetTicksNS());
}
