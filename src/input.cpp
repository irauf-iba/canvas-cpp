// input.cpp - implementation of input.hpp and output.hpp.

#include "input.hpp"
#include "output.hpp"

#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <streambuf>
#include <string>
#include <vector>

#include "internal.hpp"

namespace input {
namespace {

[[noreturn]] void fail(const std::string& message) { canvas_internal::fail("input", message); }

bool isSpace(int c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f'; }

// The stream buffer std::cin reads after fromFile(): the file, with "\r\n"
// and a lone '\r' read as '\n', a UTF-8 byte-order mark skipped, and lines
// counted for error messages. Unbuffered, with one character of lookahead.
class FileReader : public std::streambuf {
public:
    bool open(const std::string& path) {
        file_.close();
        has_ = false;
        line_ = 1;
        if (!file_.open(path, std::ios::in | std::ios::binary)) return false;
        char bom[3];
        if (file_.sgetn(bom, 3) != 3 || bom[0] != '\xEF' || bom[1] != '\xBB' || bom[2] != '\xBF') {
            file_.pubseekpos(0);
        }
        return true;
    }

    // The line the next character is on.
    int line() const { return line_; }

protected:
    int_type underflow() override { return peek(); }

    int_type uflow() override {
        int_type c = peek();
        if (c != traits_type::eof()) {
            has_ = false;
            last_ = c;
            if (c == '\n') ++line_;
        }
        return c;
    }

    int_type pbackfail(int_type c) override {
        if (has_ || last_ == traits_type::eof()) return traits_type::eof();
        if (c != traits_type::eof() && c != last_) return traits_type::eof();
        current_ = last_;
        has_ = true;
        if (last_ == '\n') --line_;
        last_ = traits_type::eof();
        return current_;
    }

private:
    int_type peek() {
        if (has_) return current_;
        int_type c = file_.sbumpc();
        if (c == '\r') {
            if (file_.sgetc() == '\n') file_.sbumpc();
            c = '\n';
        }
        current_ = c;
        has_ = c != traits_type::eof();
        return c;
    }

    std::filebuf file_;
    int_type current_ = traits_type::eof();
    int_type last_ = traits_type::eof();
    bool has_ = false;
    int line_ = 1;
};

struct State {
    FileReader reader;
    std::string fileName;  // the file fromFile() opened, or ""
};

State& st() {
    static State* s = new State;
    return *s;
}

// Reads the values of a readAll... function, counting lines, so that an error
// can say where the value was: "scores.txt, line 3" after fromFile(), or the
// line counted from where reading started otherwise.
class TokenReader {
public:
    explicit TokenReader(std::istream& in) : in_(in) {
        State& s = st();
        if (in.rdbuf() == &s.reader) {
            name_ = s.fileName;
            line_ = s.reader.line();
        }
        skipBom();
    }

    // The next word, or false at the end of the input.
    bool next(std::string& word, int& line) {
        int c = in_.peek();
        while (c != EOF && isSpace(c)) {
            in_.get();
            if (c == '\n' || (c == '\r' && in_.peek() != '\n')) ++line_;
            c = in_.peek();
        }
        if (c == EOF) return false;
        line = line_;
        word = pending_;
        pending_.clear();
        while ((c = in_.peek()) != EOF && !isSpace(c)) word += static_cast<char>(in_.get());
        return true;
    }

    std::string where(int line) const {
        return (name_.empty() ? "" : name_ + ", ") + "line " + std::to_string(line);
    }

private:
    // Skips a UTF-8 byte-order mark; partial ones are kept as part of the word.
    void skipBom() {
        const char bom[] = "\xEF\xBB\xBF";
        for (int i = 0; i < 3; ++i) {
            if (in_.peek() != static_cast<unsigned char>(bom[i])) return;
            pending_ += static_cast<char>(in_.get());
        }
        pending_.clear();
    }

    std::istream& in_;
    std::string name_;
    std::string pending_;
    int line_ = 1;
};

bool parseInt(const std::string& s, int& value, std::string& reason) {
    std::size_t i = 0;
    bool negative = false;
    if (i < s.size() && (s[i] == '+' || s[i] == '-')) negative = s[i++] == '-';
    reason = "is not a whole number";
    if (i == s.size()) return false;
    long long v = 0;
    for (; i < s.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
        v = v * 10 + (s[i] - '0');
        if (v > static_cast<long long>(std::numeric_limits<int>::max()) + 1) {
            reason = "is too large for an int";
            return false;
        }
    }
    if (negative) v = -v;
    if (v > std::numeric_limits<int>::max() || v < std::numeric_limits<int>::min()) {
        reason = "is too large for an int";
        return false;
    }
    value = static_cast<int>(v);
    return true;
}

bool parseDouble(const std::string& s, double& value, std::string& reason) {
    reason = "is not a number";
    if (s.empty() || isSpace(static_cast<unsigned char>(s[0]))) return false;
    errno = 0;
    char* end = nullptr;
    double v = std::strtod(s.c_str(), &end);
    if (end != s.c_str() + s.size()) return false;
    if (errno == ERANGE && std::abs(v) > 1) {
        reason = "is too large for a double";
        return false;
    }
    if (!std::isfinite(v)) return false;  // "nan", "inf"
    value = v;
    return true;
}

std::string inQuotes(const std::string& s) { return "'" + s + "'"; }

std::string number(double x) {
    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "%g", x);
    return buffer;
}

// Prints the prompt once and reads answers until accept() says one is valid,
// printing `problem` and the prompt again after each invalid answer.
template <typename Accept>
std::string ask(const char* function, const std::string& prompt, bool skipEmpty,
                const std::string& problem, Accept accept) {
    if (std::cin.fail() && !std::cin.eof()) std::cin.clear();  // after a failed std::cin >> x
    std::cout << prompt << std::flush;
    std::string answer;
    while (true) {
        if (!getLine(std::cin, answer)) fail(std::string(function) + ": no more input");
        if (skipEmpty && trim(answer).empty()) continue;  // also the end of a std::cin >> x line
        if (accept(answer)) return answer;
        std::cout << problem << "\n" << prompt << std::flush;
    }
}

}  // namespace

// --- working with std::cin ---------------------------------------------------------

void skipRestOfLine(std::istream& in) {
    in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void skipEmptyLines(std::istream& in) {
    for (int c = in.peek(); c == '\n' || c == '\r'; c = in.peek()) {
        in.get();
        if (c == '\r' && in.peek() == '\n') in.get();
    }
}

std::istream& getLine(std::istream& in, std::string& line) {
    if (std::getline(in, line) && !line.empty() && line.back() == '\r') line.pop_back();
    return in;
}

// --- reading everything that is left ------------------------------------------------

std::vector<int> readAllInts(std::istream& in) {
    TokenReader reader(in);
    std::vector<int> values;
    std::string word, reason;
    int line, value;
    while (reader.next(word, line)) {
        if (!parseInt(word, value, reason)) fail("readAllInts: " + reader.where(line) + ": " + inQuotes(word) + " " + reason);
        values.push_back(value);
    }
    return values;
}

std::vector<double> readAllDoubles(std::istream& in) {
    TokenReader reader(in);
    std::vector<double> values;
    std::string word, reason;
    int line;
    double value;
    while (reader.next(word, line)) {
        if (!parseDouble(word, value, reason)) {
            fail("readAllDoubles: " + reader.where(line) + ": " + inQuotes(word) + " " + reason);
        }
        values.push_back(value);
    }
    return values;
}

std::vector<std::string> readAllWords(std::istream& in) {
    TokenReader reader(in);
    std::vector<std::string> words;
    std::string word;
    int line;
    while (reader.next(word, line)) words.push_back(word);
    return words;
}

std::vector<std::string> readAllLines(std::istream& in) {
    std::vector<std::string> lines;
    std::string line;
    while (getLine(in, line)) lines.push_back(line);
    if (!lines.empty() && lines[0].compare(0, 3, "\xEF\xBB\xBF") == 0) lines[0].erase(0, 3);
    return lines;
}

// --- asking questions --------------------------------------------------------------

int askInt(const std::string& prompt) {
    int value = 0;
    std::string reason;
    ask("askInt", prompt, true, "Please enter a whole number.",
        [&](const std::string& a) { return parseInt(trim(a), value, reason); });
    return value;
}

int askInt(const std::string& prompt, int min, int max) {
    if (min > max) fail("askInt: min must not be greater than max");
    int value = 0;
    std::string reason;
    ask("askInt", prompt, true,
        "Please enter a whole number from " + std::to_string(min) + " to " + std::to_string(max) + ".",
        [&](const std::string& a) { return parseInt(trim(a), value, reason) && value >= min && value <= max; });
    return value;
}

double askDouble(const std::string& prompt) {
    double value = 0;
    std::string reason;
    ask("askDouble", prompt, true, "Please enter a number.",
        [&](const std::string& a) { return parseDouble(trim(a), value, reason); });
    return value;
}

double askDouble(const std::string& prompt, double min, double max) {
    if (!(min <= max)) fail("askDouble: min must not be greater than max");
    double value = 0;
    std::string reason;
    ask("askDouble", prompt, true, "Please enter a number from " + number(min) + " to " + number(max) + ".",
        [&](const std::string& a) { return parseDouble(trim(a), value, reason) && value >= min && value <= max; });
    return value;
}

std::string askLine(const std::string& prompt) {
    return ask("askLine", prompt, false, "", [](const std::string&) { return true; });
}

bool askYesNo(const std::string& prompt) {
    bool yes = false;
    ask("askYesNo", prompt, true, "Please answer y or n.", [&](const std::string& a) {
        std::string s = trim(a);
        for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (s == "y" || s == "yes" || s == "n" || s == "no") {
            yes = s[0] == 'y';
            return true;
        }
        return false;
    });
    return yes;
}

// --- reading a file instead of the keyboard ---------------------------------------

void fromFile(const std::string& filename) {
    const std::string path = canvas_internal::findInputFile(filename);
    if (path.empty()) fail("fromFile: " + canvas_internal::notFound(filename));
    State& s = st();
    if (!s.reader.open(path)) fail("fromFile: cannot read '" + filename + "'");
    s.fileName = filename;
    std::cin.rdbuf(&s.reader);
    std::cin.clear();
}

// --- taking strings apart -------------------------------------------------------------

std::vector<std::string> split(const std::string& s) {
    std::vector<std::string> words;
    std::string word;
    for (char c : s) {
        if (isSpace(static_cast<unsigned char>(c))) {
            if (!word.empty()) words.push_back(word);
            word.clear();
        } else {
            word += c;
        }
    }
    if (!word.empty()) words.push_back(word);
    return words;
}

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> parts(1);
    for (char c : s) {
        if (c == delimiter) {
            parts.emplace_back();
        } else {
            parts.back() += c;
        }
    }
    return parts;
}

std::string trim(const std::string& s) {
    std::size_t begin = 0, end = s.size();
    while (begin < end && isSpace(static_cast<unsigned char>(s[begin]))) ++begin;
    while (end > begin && isSpace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(begin, end - begin);
}

int toInt(const std::string& s) {
    int value;
    std::string reason;
    if (!parseInt(trim(s), value, reason)) fail("toInt: " + inQuotes(s) + " " + reason);
    return value;
}

double toDouble(const std::string& s) {
    double value;
    std::string reason;
    if (!parseDouble(trim(s), value, reason)) fail("toDouble: " + inQuotes(s) + " " + reason);
    return value;
}

}  // namespace input

namespace output {

void toFile(const std::string& filename) {
    std::cout.flush();
    std::fflush(stdout);
    // freopen redirects the C stream, so std::cout (which writes through it)
    // and printf both go to the file.
    if (!std::freopen(filename.c_str(), "w", stdout)) {
        canvas_internal::fail("output", "toFile: cannot write '" + filename + "'");
    }
}

}  // namespace output
