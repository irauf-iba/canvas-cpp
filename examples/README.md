# Examples

Small programs that show the library at work: first one for each main
feature, then classic programs from the Princeton textbook. Each file
explains at the top what it does and what arguments it takes.

## Library features

| Program | Shows | Library |
|---|---|---|
| [`shapes`](shapes.cpp) | One of each kind of shape | `canvas` |
| [`bouncing_ball`](bouncing_ball.cpp) | The animation loop: double buffering and a frame rate | `canvas` |
| [`sketch`](sketch.cpp) | Drawing with the mouse; keys to change colour, clear and save | `canvas` |
| [`paddle`](paddle.cpp) | A small game: a start button, a ball hitting a paddle, sound effects, colours from `hsv`, debug keys, and recording a GIF (`./paddle record`) | `canvas`, `audio` |
| [`koch`](koch.cpp) | A Koch snowflake, asking for the depth | `turtle`, `input` |
| [`tree`](tree.cpp) | A recursive tree, with branches getting thinner and turning green | `turtle` |
| [`image_effects`](image_effects.cpp) | Grayscale and mirror images, pixel by pixel, of a drawing or a photo | `image`, `canvas` |
| [`scale`](scale.cpp) | A C major scale played one sample at a time, then saved to a file | `audio` |
| [`piano`](piano.cpp) | A keyboard piano: type keys or click them, with notes in the background | `audio`, `canvas` |
| [`random_walk`](random_walk.cpp) | A random walk that prints its seed, to repeat it | `chance`, `canvas` |
| [`average`](average.cpp) | The average, median, spread and extremes of numbers on standard input | `input`, `stats` |
| [`grades`](grades.cpp) | Reading `name,score` lines and printing grades | `input` |
| [`guess`](guess.cpp) | Guess the number, with questions that keep asking until the answer is valid | `input`, `chance` |

## From the textbook

Classic programs from Sedgewick and Wayne's *Computer Science: An
Interdisciplinary Approach*, the textbook of Princeton's introductory course.
Each file names the section of the book it follows.

They are new programs for the same exercises, not translations of the
booksite code. Programs that are usually set as assignments, such as
Mandelbrot, N-body, Guitar Hero and Percolation, are left out on purpose.

| Program | Book | Shows | Library |
|---|---|---|---|
| [`function_graph`](function_graph.cpp) | 1.5 | Plotting a function from an array of points; too few points give a wrong picture | `canvas` |
| [`plot_filter`](plot_filter.cpp) | 1.5 | Reading points from standard input and plotting them: a map of the world, or of Pakistan | `std::cin`, `canvas` |
| [`play_that_tune`](play_that_tune.cpp) | 1.5 | Reading notes from standard input and synthesizing them | `std::cin`, `audio` |
| [`chaos_game`](chaos_game.cpp) | 2.2 | The Sierpinski triangle appearing from random jumps | `chance`, `canvas` |
| [`bernoulli`](bernoulli.cpp) | 2.2 | Coin flips as a histogram, approaching the normal curve | `chance`, `stats`, `canvas` |
| [`htree`](htree.cpp) | 2.3 | The H-tree fractal, by recursion | `canvas` |
| [`brownian`](brownian.cpp) | 2.3 | A Brownian bridge (a "mountain range"), by recursion with random moves | `chance`, `canvas` |
| [`spiral`](spiral.cpp) | 3.2 | A turtle drawing polygons with shrinking sides | `turtle` |
| [`drunken_turtle`](drunken_turtle.cpp) | 3.2 | A turtle taking random steps, animated | `turtle`, `chance` |

## Running them

Build the repository (see the [README](../README.md)); the programs are in
`build/examples/`. Many take optional arguments, listed at the top of each
file:

```sh
./build/examples/htree 7
./build/examples/spiral 3 0.95
./build/examples/bernoulli 50 100000
```

Some read standard input, so give them a data file with `<`:

```sh
./build/examples/average < examples/data/numbers.txt
./build/examples/grades < examples/data/scores.csv
./build/examples/plot_filter < examples/data/cities.txt
./build/examples/plot_filter < examples/data/pakistan_cities.txt
./build/examples/play_that_tune < examples/data/ode_to_joy.txt
```

Programs that use random numbers print their seed, so a result you like can
be repeated with `CANVAS_SEED=<seed>`.

## Data files

The data files were made for this repository:

- [`data/numbers.txt`](data/numbers.txt): a few numbers, for `average`.
- [`data/scores.csv`](data/scores.csv): `name,score` lines, for `grades`.
- [`data/cities.txt`](data/cities.txt): the first line is the bounding box
  (`-180 -90 180 90`); each line after it is the longitude and latitude of a
  city, 1251 in all, from [Natural Earth](https://www.naturalearthdata.com)'s
  populated places (public domain).
- [`data/pakistan_cities.txt`](data/pakistan_cities.txt): the same format, for
  the 570 places in Pakistan with at least 1000 people. Selected from the
  GeoNames `cities1000` file, by [GeoNames](https://www.geonames.org), licensed
  under [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/); only the
  coordinates are kept, rounded to four decimals.
- [`data/ode_to_joy.txt`](data/ode_to_joy.txt): one note per line, a pitch in
  semitones from A4 (440 Hz) and a duration in seconds; the melody is
  Beethoven's (public domain). Write your own tune in the same format.
