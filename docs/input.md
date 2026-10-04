# input and output: help with std::cin and std::cout

`#include <input.hpp>` and `#include <output.hpp>`

Programs read input with `std::cin` and write output with `std::cout`, as in
any C++ program. The `input` module adds help where `std::cin` trips
beginners up, and the `output` module sends output to a file from an IDE.

As in Princeton's course, a program can read a file through standard input,
and write its output to a file, by redirection in the shell:

```sh
./average < numbers.txt              # std::cin reads numbers.txt
./average < numbers.txt > result.txt # and std::cout writes result.txt
```

The headers [`include/input.hpp`](../include/input.hpp) and
[`include/output.hpp`](../include/output.hpp) document every function.

## The std::cin traps, and the helpers for them

### Reading a line after `std::cin >> x`

After `std::cin >> age`, the end of that line is still waiting, so a following
`std::getline` reads an empty line instead of the next one. Call
`input::skipRestOfLine()` in between:

```cpp
int age;
std::cin >> age;
input::skipRestOfLine();             // throws away the rest of the age line
std::cout << "Name? ";
std::string name;
input::getLine(std::cin, name);
```

`skipRestOfLine()` never waits for more input, so it is safe right before a
prompt.

### Windows line endings

A file saved on Windows ends every line with `"\r\n"`. On Linux and macOS,
`std::getline` leaves the `'\r'` at the end of each line, so `line == "yes"`
is false and printing looks wrong. `input::getLine(in, line)` works exactly
like `std::getline`, without the `'\r'`:

```cpp
std::string line;
while (input::getLine(std::cin, line)) {
    // ...
}
```

### Wrong input that silently ends a loop

`while (std::cin >> x)` stops at the first value that isn't a number, without
any message, so a program quietly works on half the data. `readAll…` reads
everything that is left, and stops with a message saying where the bad value
is:

```cpp
std::vector<double> values = input::readAllDoubles();
// input: readAllDoubles: line 3: '1,5' is not a number
```

### Asking a question and checking the answer

Checking `std::cin >> choice` properly takes `fail()`, `clear()` and
`ignore()`. The `ask…` functions print a prompt and keep asking until the
answer is valid:

```cpp
std::cout << "1) Add  2) List  3) Quit\n";
int choice = input::askInt("Choice? ", 1, 3);
```

```
1) Add  2) List  3) Quit
Choice? five
Please enter a whole number from 1 to 3.
Choice? 2
```

After a wrong answer the whole prompt is printed again, so keep menus and
other long text out of it.

## Functions

### Working with std::cin

| Function | Does |
|---|---|
| `input::skipRestOfLine()` | Throws away the rest of the current line. Use it after `std::cin >> x`, before reading a line. |
| `input::skipEmptyLines()` | Skips completely empty lines, for data files with blank lines between records. (On the keyboard it waits for the next line, so don't use it right before a prompt.) |
| `input::getLine(in, line)` | Like `std::getline(in, line)`, without the Windows `'\r'`. |

They work on `std::cin` unless another stream is given, e.g.
`input::skipRestOfLine(file)`.

### Reading everything that is left

| Function | Gives |
|---|---|
| `input::readAllInts()` | All the remaining whole numbers, as a `std::vector<int>`. |
| `input::readAllDoubles()` | All the remaining numbers, as a `std::vector<double>`. |
| `input::readAllWords()` | All the remaining words, separated by spaces or new lines. |
| `input::readAllLines()` | All the remaining lines, including empty ones, without `'\r'`. |

They read `std::cin` unless another stream is given:
`input::readAllInts(file)`.

### Asking questions

| Function | Gives |
|---|---|
| `input::askInt(prompt)`, `input::askInt(prompt, min, max)` | A whole number, within the range if one is given. |
| `input::askDouble(prompt)`, `input::askDouble(prompt, min, max)` | A number. |
| `input::askLine(prompt)` | The answer as typed, which may be empty. |
| `input::askYesNo(prompt)` | `true` for y or yes, `false` for n or no, in upper or lower case. |

Each reads a whole line as the answer, so a wrong answer never leaves junk
behind for the next question. Empty lines are ignored, except by `askLine`,
and that also takes care of the end of a line left by `std::cin >> x`. Before
`askLine`, call `skipRestOfLine()` first. If the input ends, for example when
a file is redirected in, the program stops with
`input: askInt: no more input` instead of asking forever.

### Taking strings apart

| Function | Gives |
|---|---|
| `input::split(s)` | The words of `s`: `split("  to be  or ")` gives `{"to", "be", "or"}`. |
| `input::split(s, ',')` | The parts between commas, including empty ones: `split("Ali,23,,B+", ',')` gives `{"Ali", "23", "", "B+"}`. |
| `input::trim(s)` | `s` without spaces at either end. |
| `input::toInt(s)`, `input::toDouble(s)` | The number in `s`, or a clear error such as `input: toInt: '3.5' is not a whole number`. |

Reading a comma-separated file, one record per line:

```cpp
std::string line;
while (input::getLine(std::cin, line)) {
    std::vector<std::string> fields = input::split(line, ',');
    std::string name = input::trim(fields[0]);
    int score = input::toInt(fields[1]);
}
```

## In an IDE: fromFile and toFile

Redirecting with `<` and `>` is easy in a terminal but awkward in most IDEs.
One line at the start of `main()` does the same:

```cpp
input::fromFile("numbers.txt");   // std::cin reads numbers.txt
output::toFile("result.txt");     // std::cout and printf write result.txt
```

The rest of the program is unchanged: `std::cin >> x`, `std::getline`,
`std::cout` and the helpers above all use the files. Delete the lines to go
back to the keyboard and the screen.

- **`input::fromFile`** looks for the file in the current folder and then next
  to the program, like every file the library reads. While reading it,
  Windows line endings read as `'\n'`, a byte-order mark at the start is
  skipped, and error messages name the file and line, e.g.
  `input: readAllInts: numbers.txt, line 3: ...`. It affects `std::cin`, not
  C's `scanf`.
- **`output::toFile`** creates the file, or empties it if it exists. Calling it
  again switches to another file.

### Data from the web

The library doesn't read from URLs. Download the file once with `curl`, which
comes with Windows 10 and 11, macOS and Linux, and read it with `fromFile`
(or `<`):

```sh
curl -fL -o cities.txt https://raw.githubusercontent.com/irauf-iba/canvas-cpp/v0.1/examples/data/cities.txt
```

On Windows, type `curl.exe` in PowerShell, where `curl` alone means something
else. Then `input::fromFile("cities.txt")` reads it, and every run uses the
same data, even without a network.

## Errors

Mistakes stop the program with a message and exit status 1, for example:

```
input: readAllInts: line 3: 'abc' is not a whole number
input: readAllInts: numbers.txt, line 1: '99999999999' is too large for an int
input: askInt: no more input
input: toDouble: 'abc' is not a number
input: fromFile: cannot open 'numbers.txt' (not in the current folder, /home/ana/lab4/build/)
output: toFile: cannot write 'results/out.txt'
```

Without `fromFile`, the line number counts from where `readAll…` started
reading.

## Compared with Princeton's StdIn and StdOut

Princeton's StdIn replaces Java's awkward input classes. In C++, `std::cin`
already reads values one at a time, so `input` doesn't replace it: it adds
what `std::cin` lacks.

| Princeton | C++ with this library |
|---|---|
| `StdIn.readInt()`, `StdIn.readString()` | `std::cin >> x` |
| `StdIn.isEmpty()` | `while (std::cin >> x)`, or `input::readAll…()` |
| `StdIn.readLine()` | `input::getLine(std::cin, line)` |
| `StdIn.readAllInts()`, `readAllStrings()`, `readAllLines()` | `input::readAllInts()`, `readAllWords()`, `readAllLines()`, with errors for bad values |
| `In` objects for named files | `input::fromFile(name)`, then `std::cin` |
| `In` objects for URLs | Download with `curl`, then `input::fromFile(name)` ([above](#data-from-the-web)) |
| `StdOut.println(x)` | `std::cout << x << "\n"`; `output::toFile(name)` for a file |
| — | `skipRestOfLine`, `skipEmptyLines`, `ask…`, `split`, `trim`, `toInt`, `toDouble` |

## How it works

- **`fromFile`** gives `std::cin` a stream buffer that reads the file, turns
  `"\r\n"` into `'\n'`, skips a byte-order mark and counts lines. Code that
  uses `std::cin` doesn't notice the difference.
- **`output::toFile`** reopens the C standard output stream on the file, so
  `std::cout`, which writes through it, and `printf` both go there.
- **The `ask…` functions** read whole lines with `getLine` and check them
  with the same parsing as `toInt` and `toDouble`. An earlier failed
  `std::cin >> x` is cleared first, so asking works after it.

## Examples

- [`examples/average.cpp`](../examples/average.cpp): the average, median,
  standard deviation, smallest and largest of numbers on standard input,
  with [stats](stats.md):
  `./average < examples/data/numbers.txt`.
- [`examples/grades.cpp`](../examples/grades.cpp): reads `name,score` lines
  with `getLine`, `split` and `toInt`:
  `./grades < examples/data/scores.csv`.
- [`examples/guess.cpp`](../examples/guess.cpp): a number-guessing game with
  `askInt`, `askYesNo` and `chance`.
