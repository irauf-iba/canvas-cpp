# image: images as grids of pixels

`#include <image.hpp>`

The `image` module is for image-processing exercises, like Princeton's
[Picture](https://introcs.cs.princeton.edu/java/stdlib/javadoc/Picture.html).
An `image::Image` is a plain struct: you can copy it, pass it to functions and
return it from them. The module needs no window, so image programs can run
anywhere, including on a grading server.

```cpp
#include <draw.hpp>
#include <image.hpp>

int main() {
    image::Image src = image::load("photo.png");
    image::Image out = image::create(src.width, src.height);
    for (int row = 0; row < src.height; ++row) {
        for (int col = 0; col < src.width; ++col) {
            image::Color c = image::get(src, col, row);
            int gray = (299 * c.r + 587 * c.g + 114 * c.b) / 1000;
            image::set(out, col, row, image::rgb(gray, gray, gray));
        }
    }
    image::save(out, "gray.png");

    draw::setCanvasSize(out.width, out.height);   // and show it
    draw::picture(0.5, 0.5, out);
}
```

The header [`include/image.hpp`](../include/image.hpp) documents every function.

## The Image struct

```cpp
struct Image {
    int width = 0;
    int height = 0;
    std::vector<Color> pixels;   // row after row, top row first
};
```

Pixels are addressed by column and row. Column 0 is at the left and row 0 at
the top, as in image files and image editors. This is the opposite of `draw`'s
y axis, which points up. The difference is deliberate: rows and columns are
positions in a grid, not coordinates.

Pixel (col, row) is `pixels[row * width + col]`. Prefer `get` and `set`, which
check that the pixel is inside the image.

## Functions

| Function | Does |
|---|---|
| `image::create(width, height)` | A new white image. |
| `image::create(width, height, color)` | A new image filled with the color. |
| `image::load(file)` | Reads a .png, .jpg, .bmp or .gif (first frame) file. |
| `image::save(img, file)` | Writes .png, .jpg or .bmp, chosen by the extension. PNG and BMP keep transparency; JPEG does not. |
| `image::get(img, col, row)` | The color of a pixel. |
| `image::set(img, col, row, color)` | Changes the color of a pixel. |

## Colors

`image::Color` is the same type as `draw::Color`, from
[`include/color.hpp`](../include/color.hpp). It has `r`, `g`, `b` and `a`
(opacity) components from 0 to 255:

- Make one from `int` values with `image::rgb(r, g, b)`, which clamps to
  0–255.
- Compare colors with `==`.
- The predefined colors are available under both names, e.g. `image::RED` is
  `draw::RED`. [draw.md](draw.md#pen-colors-and-text) lists them.

## Using images with draw

| Function | Does |
|---|---|
| `draw::picture(x, y, img)` | Draws the image centered at (x, y), at its natural size. Each image pixel covers exactly one canvas pixel. |
| `draw::picture(x, y, img, width, height)` | The same, scaled to a width and height in user coordinates. |
| `draw::canvas()` | A copy of the canvas as an image, e.g. to process a drawing. |

On a canvas the same size as an image, `draw::picture(0.5, 0.5, img)` fills
the canvas exactly (with the default scale), reproducing the image pixel for
pixel.

## Errors

Mistakes stop the program with a message and exit status 1, for example:

```
image: get: col 640 is outside the image (0 to 639)
image: set: row -1 is outside the image (0 to 479)
image: load: cannot open 'photo.png' (can't fopen)
image: save: 'out.gif' must end in .png, .jpg or .bmp
draw: picture: the image has 3 pixels, but width x height is 2 x 2
```

The last one appears if a program changes `width`, `height` or `pixels` by
hand so that they no longer match.

## Differences from Princeton's Picture

| Picture | image | Why |
|---|---|---|
| `Picture` class with `get` and `set` methods | `image::Image` struct with `image::get` and `image::set` | Same idea without classes. Images are values you can copy and return. |
| `picture.show()` opens a window | `draw::picture(x, y, img)` | Images are shown on the `draw` canvas, together with any drawing. |
| `setOriginLowerLeft()` | Always top-left | One convention, the same as image files and editors. |
| Exceptions | Message and exit | Clearer for beginners than an uncaught exception. |

## How it works

- **Files.** Reading and writing use stb_image and stb_image_write. They are
  compiled into the library privately, so they can't clash with a copy of stb
  in a student's program.
- **No SDL.** The module doesn't use SDL, and image programs don't need a
  display.
- **Speed.** `get` and `set` check their arguments on every call and are still
  fast: a grayscale pass over a 1000 × 1000 image takes about 15 ms.

## Example

[`examples/image_effects.cpp`](../examples/image_effects.cpp) shows a grayscale
and a mirrored copy side by side. It uses a drawing it makes itself, or an
image file you pass to it: `./image_effects photo.png`.
