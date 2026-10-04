// The grid and watched values are drawn over the canvas when it is shown, but
// never into it: snapshot() and save() are unchanged. Runs headless; what the
// window would show comes from canvas_internal::screenImage().

#include <canvas.hpp>
#include <image.hpp>

#include <string>

#include "check.hpp"
#include "internal.hpp"

int main() {
    canvas::setCanvasSize(200, 150);
    canvas::setPenColor(canvas::BOOK_BLUE);
    canvas::filledCircle(0.5, 0.5, 0.2);
    const image::Image drawing = canvas::snapshot();
    CHECK(canvas_internal::screenImage().pixels == drawing.pixels);  // no overlay yet

    canvas::showGrid(0.25);
    canvas::watch("x", 42);
    canvas::watch("name", "Ali");
    canvas::watch("ok", true);
    canvas::watch("x", 43.5);  // updated in place, still first
    CHECK(canvas::snapshot().pixels == drawing.pixels);           // the canvas is untouched
    CHECK(canvas_internal::screenImage().pixels != drawing.pixels);  // but the screen shows more

    canvas::hideGrid();
    image::Image watchesOnly = canvas_internal::screenImage();
    CHECK(watchesOnly.pixels != drawing.pixels);

    canvas::unwatch("x");
    canvas::unwatch("name");
    canvas::unwatch("ok");
    canvas::unwatch("not watched");  // ignored
    CHECK(canvas_internal::screenImage().pixels == drawing.pixels);  // all gone again
    return finish();
}
