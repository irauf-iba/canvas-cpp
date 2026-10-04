# Getting started

This guide takes you from nothing to a running program that draws in a
window. You need a C++ compiler, CMake, and the starter project in the
[`template`](../template) folder.

> The Linux steps have been tested. The Windows and macOS steps are the usual
> ones for these tools but have not yet been tried with this library; please
> report anything that doesn't work.

## 1. Install the tools

**Windows.** Install [Visual Studio Community](https://visualstudio.microsoft.com/vs/community/)
and, in the installer, choose the **Desktop development with C++** workload.
It includes the compiler and CMake.

**macOS.** In Terminal, run `xcode-select --install` for the compiler. Then
install CMake from [cmake.org](https://cmake.org/download/), or with Homebrew:
`brew install cmake`.

**Linux (Debian/Ubuntu).** Install the compiler, CMake, and the libraries
needed to build the window and sound support:

```sh
sudo apt install build-essential cmake ninja-build pkg-config \
  libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev \
  libxss-dev libxtst-dev libxkbcommon-dev libwayland-dev wayland-protocols \
  libdecor-0-dev libegl1-mesa-dev libgl1-mesa-dev \
  libasound2-dev libpulse-dev libpipewire-0.3-dev
```

## 2. Get the starter project

Copy the [`template`](../template) folder and give it your own name. Its
[`README.md`](../template/README.md) is a one-page version of this guide. It
contains:

- `main.cpp`: a small program (a ball that follows the mouse) to replace with
  your own;
- `README.md`: the short version of this guide;
- `CMakeLists.txt`: the build instructions, which download the library;
- `data/`: a folder for images and sounds your program opens.

## 3. Build and run

The first build downloads and compiles the library and SDL, which takes a
minute or two and needs an internet connection. Later builds are fast.

**Visual Studio (Windows).** Choose **File → Open → Folder** and open your
project folder. Visual Studio reads `CMakeLists.txt` by itself. When it has
finished, choose **main.exe** in the **Select Startup Item** list on the
toolbar and press **F5**.

**Visual Studio Code.** Install the **C/C++** and **CMake Tools** extensions.
Open your project folder, pick a compiler ("kit") when asked, then click
**Build** and the **Run** (▷) button in the status bar.

**CLion.** Open your project folder. CLion loads the CMake project by itself;
choose the **main** configuration and click **Run**.

**Command line.** In your project folder:

```sh
cmake -S . -B build
cmake --build build
./build/main            # Windows: build\Debug\main.exe
```

A window should open with a blue ball that follows the mouse.

## 4. Write your program

Edit `main.cpp`. The guides explain each library:

- [canvas](canvas.md): drawing, animation, mouse and keyboard;
- [turtle](turtle.md): drawing by moving and turning;
- [image](image.md): images, pixel by pixel;
- [audio](audio.md): sound;
- [chance](chance.md): random numbers;
- [stats](stats.md): averages, spread and quick plots of numbers;
- [stopwatch](stopwatch.md): timing code;
- [input](input.md): help with `std::cin`, and asking questions with checked answers.

To split your program into several files, add the other `.cpp` files to the
`add_executable` line in `CMakeLists.txt`:
`add_executable(main main.cpp ball.cpp)`.

Use the library's names with their namespace, as in `canvas::circle` and
`image::load`. That keeps them apart from names in your own program and in
`std`.

## 5. Images and sounds

Put files that your program opens in the `data` folder, and open them by name,
e.g. `canvas::picture(0.5, 0.5, "cat.png")`. The files are copied next to the
program each time it is built. The library looks for a file in the current
folder first, then next to the program, so it finds them however the program
is started.

Files your program saves, such as `canvas::save("drawing.png")`, go into the
current folder. With an IDE that is usually the build folder, e.g.
`build/` or `out/build/...`.

## Troubleshooting

- **The first build fails while downloading.** The build needs internet
  access the first time. In a lab without it, ask for a copy of the library
  and SDL; see "Offline labs" in the [README](../README.md).
- **`canvas: cannot open a window`.** The program runs without a display, for
  example over SSH. Run it on a desktop, or set `CANVAS_HEADLESS=1` to draw
  without a window and save the result with `canvas::save`.
- **`audio: no audio device ...; continuing without sound`.** There is no
  sound device. On Linux, if this happens on a desktop, the sound development
  packages from step 1 were missing when the library was first built: install
  them, delete the `build` folder and build again.
- **`... cannot open 'cat.png' (not in the current folder, ...)`.** The file
  isn't in either folder that was searched, and the message names both. Check
  that the file is in `data/`, and that the name and extension match exactly
  (`Cat.PNG` is a different name from `cat.png` on Linux and macOS).
- **Nothing happens when I close the window.** Closing the window ends the
  program. If it doesn't, the program is busy in a long calculation without
  drawing; press Ctrl+C in the terminal, or stop it from the IDE.
