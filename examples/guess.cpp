// Guess the number: the computer picks a number from 1 to 100 and says
// whether each guess is too high or too low. Shows the input::ask...
// functions, which keep asking until the answer is valid.

#include <chance.hpp>
#include <input.hpp>

#include <iostream>

int main() {
    std::cout << "I'm thinking of a number from 1 to 100.\n";
    do {
        int secret = chance::uniform(1, 101);
        int tries = 0, guess = 0;
        while (guess != secret) {
            guess = input::askInt("Your guess? ", 1, 100);
            ++tries;
            if (guess < secret) std::cout << "Too low.\n";
            if (guess > secret) std::cout << "Too high.\n";
        }
        std::cout << "Right! You took " << tries << " tries.\n";
    } while (input::askYesNo("Play again? "));
}
