# Textbook examples

Classic programs from Sedgewick and Wayne's *Computer Science: An
Interdisciplinary Approach* (the textbook of Princeton's introductory course),
written for this library. Each file names the section of the book it follows.

They are new programs for the same exercises, not translations of the
booksite code, and the data files were made for this repository: the cities
come from [Natural Earth](https://www.naturalearthdata.com) (public domain),
and the melody is Beethoven's (public domain). Programs that are usually set as
assignments, such as Mandelbrot, N-body, Guitar Hero and Percolation, are left
out on purpose.

| Program | Book | Shows | Library |
|---|---|---|---|
| [`function_graph`](function_graph.cpp) | 1.5 | Plotting a function from an array of points; too few points give a wrong picture | `canvas` |
| [`plot_filter`](plot_filter.cpp) | 1.5 | Reading points from standard input and plotting them: a map of the world | `std::cin`, `canvas` |
| [`play_that_tune`](play_that_tune.cpp) | 1.5 | Reading notes from standard input and synthesizing them | `std::cin`, `audio` |
| [`chaos_game`](chaos_game.cpp) | 2.2 | The Sierpinski triangle appearing from random jumps | `chance`, `canvas` |
| [`bernoulli`](bernoulli.cpp) | 2.2 | Coin flips as a histogram, approaching the normal curve | `chance`, `canvas` |
| [`htree`](htree.cpp) | 2.3 | The H-tree fractal, by recursion | `canvas` |
| [`brownian`](brownian.cpp) | 2.3 | A Brownian bridge (a "mountain range"), by recursion with random moves | `chance`, `canvas` |
| [`spiral`](spiral.cpp) | 3.2 | A turtle drawing polygons with shrinking sides | `turtle` |
| [`drunken_turtle`](drunken_turtle.cpp) | 3.2 | A turtle taking random steps, animated | `turtle`, `chance` |

## Running them

Build the repository (see the [README](../../README.md)); the programs are
in `build/examples/textbook/`. Most take optional arguments, listed at the top
of each file:

```sh
./build/examples/textbook/htree 7
./build/examples/textbook/spiral 3 0.95
./build/examples/textbook/bernoulli 50 100000
```

Two read standard input, so give them a data file with `<`:

```sh
./build/examples/textbook/plot_filter < examples/textbook/data/cities.txt
./build/examples/textbook/play_that_tune < examples/textbook/data/ode_to_joy.txt
```

Programs that use random numbers print their seed, so a result you like can
be repeated with `CANVAS_SEED=<seed>`.

## Data files

- [`data/cities.txt`](data/cities.txt): the first line is the bounding box
  (`-180 -90 180 90`); each line after it is the longitude and latitude of a
  city, 1251 in all.
- [`data/ode_to_joy.txt`](data/ode_to_joy.txt): one note per line, a pitch in
  semitones from A4 (440 Hz) and a duration in seconds. Write your own tune in
  the same format.
