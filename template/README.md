# Your program with canvas

This folder is a starter project. It shows a ball that follows the mouse;
replace it with your own program. You need three things: a C++ compiler,
CMake, and an internet connection for the first build.

## 1. Install the tools (once)

- **Windows:** install [Visual Studio Community](https://visualstudio.microsoft.com/vs/community/)
  with the **Desktop development with C++** workload.
- **macOS:** run `xcode-select --install` in Terminal, then install
  [CMake](https://cmake.org/download/) (or `brew install cmake`).
- **Linux (Debian/Ubuntu):**

  ```sh
  sudo apt install build-essential cmake ninja-build pkg-config \
    libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev \
    libxss-dev libxtst-dev libxkbcommon-dev libwayland-dev wayland-protocols \
    libdecor-0-dev libegl1-mesa-dev libgl1-mesa-dev \
    libasound2-dev libpulse-dev libpipewire-0.3-dev
  ```

## 2. Copy this folder

Make a copy of this folder for each program, and give it your own name.

## 3. Build and run

Open the folder in your editor:

- **Visual Studio:** File → Open → Folder, choose **main.exe** as the startup
  item, press **F5**.
- **VS Code:** with the **C/C++** and **CMake Tools** extensions, open the
  folder, then **Build** and **Run** (▷) in the status bar.
- **CLion:** open the folder and click **Run**.

Or use the command line:

```sh
cmake -S . -B build
cmake --build build
./build/main            # Windows: build\Debug\main.exe
```

The first build downloads and compiles the library, which takes a minute or
two. A window with a blue ball should appear. Close it to end the program.

## 4. Write your program

Edit `main.cpp`. Include what you need, and write the library's names with
their namespace:

```cpp
#include <canvas.hpp>   // drawing, animation, mouse and keyboard
#include <turtle.hpp>   // drawing by moving and turning
#include <image.hpp>    // images, pixel by pixel
#include <audio.hpp>    // sound
#include <chance.hpp>   // random numbers
#include <stats.hpp>    // mean, median, stddev, and quick plots
#include <input.hpp>    // help with std::cin: input::askInt and more

int main() {
    canvas::setPenColor(canvas::BOOK_BLUE);
    canvas::filledCircle(0.5, 0.5, 0.25);   // x, y, radius; (0, 0) is bottom left
}
```

- **More files:** list every `.cpp` file in `CMakeLists.txt`, e.g.
  `add_executable(main main.cpp ball.cpp)`.
- **Images and sounds:** put them in the `data` folder and open them by name,
  e.g. `canvas::picture(0.5, 0.5, "cat.png")`.

## When something goes wrong

The library stops with a message that says what happened, for example
`canvas: filledCircle: radius must not be negative`. Read it first.

- **"cannot open 'cat.png'":** the file isn't in `data`, or the name differs
  (on Linux and macOS, `Cat.PNG` and `cat.png` are different names).
- **The first build fails:** check your internet connection.
- **No sound on Linux:** the sound packages from step 1 were missing when you
  first built. Install them, delete the `build` folder and build again.

The library guides explain every function:

- [canvas](https://github.com/irauf-iba/canvas-cpp/blob/main/docs/canvas.md): drawing, animation, mouse and keyboard
- [turtle](https://github.com/irauf-iba/canvas-cpp/blob/main/docs/turtle.md): drawing by moving and turning
- [image](https://github.com/irauf-iba/canvas-cpp/blob/main/docs/image.md): images, pixel by pixel
- [audio](https://github.com/irauf-iba/canvas-cpp/blob/main/docs/audio.md): sound
- [chance](https://github.com/irauf-iba/canvas-cpp/blob/main/docs/chance.md): random numbers
- [stats](https://github.com/irauf-iba/canvas-cpp/blob/main/docs/stats.md): averages, spread and quick plots
- [input](https://github.com/irauf-iba/canvas-cpp/blob/main/docs/input.md): help with `std::cin`, and asking questions

More detail on setup and problems is in
[getting started](https://github.com/irauf-iba/canvas-cpp/blob/main/docs/getting-started.md).
