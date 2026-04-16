// cpp
// main.cpp
// Sudoku console game (C++17)
// Compile: g++ -std=c++17 main.cpp -O2 -o sudoku
// Run: ./sudoku
//
// Terminal must support ANSI and Unicode. This uses POSIX termios (Linux/macOS).

#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include <random>
#include <chrono>
#include <stack>
#include <optional>
#include <sstream>
#include <string>
#include <functional>
#include <unordered_set>
#include <thread>
#include <termios.h>
#include <unistd.h>
#include <cctype>

using namespace std::chrono_literals;

// ---------- ANSI / UI helpers ----------
namespace ansi {
    static constexpr const char* RESET = "\x1b[0m";
    static constexpr const char* BOLD = "\x1b[1m";
    static constexpr const char* FG_CYAN = "\x1b[36m";
    static constexpr const char* FG_WHITE = "\x1b[97m";
    static constexpr const char* FG_GREEN = "\x1b[92m";
    static constexpr const char* FG_RED = "\x1b[31m";
    static constexpr const char* FG_YELLOW = "\x1b[33m";
    static constexpr const char* BG_BLUE = "\x1b[48;5;19m";
    static constexpr const char* CLEAR_SCREEN = "\x1b[2J\x1b[H";
    inline void sleep_ms(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
}

// Unicode box drawing pieces for 3x3 bold subgrid borders
namespace box {
    const std::string TL = "╔", TR = "╗", BL = "╚", BR = "╝";
    const std::string H = "═", V = "║";
    const std::string T = "╦", B = "╩", L = "╠", R = "╣", C = "╬";
    const std::string I_H = "─", I_V = "│", I_T = "┬", I_B = "┴", I_L = "├", I_R = "┤", I_C = "┼";
}

// ---------- Types ----------
enum class Difficulty { Easy, Medium, Hard };

struct Move { int r, c, prev; };

// ---------- SudokuBoard: generation, solver, validation ----------
class SudokuBoard {
public:
    SudokuBoard() { reset(); }

    void reset() {
        for (auto &row : grid) row.fill(0);
        for (auto &row : fixed) row.fill(false);
        solutionFilled = false;
    }

    void generate(Difficulty diff) {
        reset();
        fillFullBoard();
        for (int r = 0; r < 9; ++r) for (int c = 0; c < 9; ++c) solution[r][c] = grid[r][c];
        solutionFilled = true;
        int clues = difficultyClues(diff);
        removeUntilClues(clues);
        for (int r = 0; r < 9; ++r) for (int c = 0; c < 9; ++c) fixed[r][c] = (grid[r][c] != 0);
    }

    bool canPlace(int r, int c, int v) const {
        if (r < 0 || r >= 9 || c < 0 || c >= 9) return false;
        if (v < 1 || v > 9) return false;
        if (grid[r][c] != 0 && grid[r][c] != v) return false;
        for (int i = 0; i < 9; ++i) if (grid[r][i] == v && i != c) return false;
        for (int i = 0; i < 9; ++i) if (grid[i][c] == v && i != r) return false;
        int br = (r/3)*3, bc = (c/3)*3;
        for (int i = 0; i < 3; ++i) for (int j = 0; j < 3; ++j) {
            int rr = br + i, cc = bc + j;
            if ((rr != r || cc != c) && grid[rr][cc] == v) return false;
        }
        return true;
    }

    bool placeValue(int r, int c, int v) {
        if (r < 0 || r >= 9 || c < 0 || c >= 9) return false;
        if (fixed[r][c]) return false;
        if (v == 0) { grid[r][c] = 0; return true; }
        if (!canPlace(r, c, v)) return false;
        grid[r][c] = v;
        return true;
    }

    bool isSolved() const {
        for (int r = 0; r < 9; ++r) for (int c = 0; c < 9; ++c) if (grid[r][c] == 0) return false;
        // verify validity
        for (int r = 0; r < 9; ++r) for (int c = 0; c < 9; ++c) {
            int v = grid[r][c];
            if (v < 1 || v > 9) return false;
            // temporarily clear and check
            int tmp = grid[r][c];
            // check row/col/box excluding self
            for (int i = 0; i < 9; ++i) if (i != c && grid[r][i] == v) return false;
            for (int i = 0; i < 9; ++i) if (i != r && grid[i][c] == v) return false;
            int br = (r/3)*3, bc = (c/3)*3;
            for (int i=0;i<3;++i) for (int j=0;j<3;++j) {
                int rr = br+i, cc = bc+j;
                if ((rr!=r || cc!=c) && grid[rr][cc]==v) return false;
            }
            (void)tmp;
        }
        return true;
    }

    std::optional<Move> hintOne() {
        if (!solutionFilled) return {};
        std::vector<std::pair<int,int>> empties;
        for (int r=0;r<9;++r) for (int c=0;c<9;++c) if (grid[r][c]==0) empties.emplace_back(r,c);
        if (empties.empty()) return {};
        std::shuffle(empties.begin(), empties.end(), rng);
        auto [rr,cc] = empties.front();
        int solv = solution[rr][cc];
        Move mv{rr,cc,0};
        grid[rr][cc] = solv;
        return mv;
    }

    int at(int r,int c) const { if (r<0||r>=9||c<0||c>=9) return 0; return grid[r][c]; }
    bool isFixed(int r,int c) const { if (r<0||r>=9||c<0||c>=9) return false; return fixed[r][c]; }
    void set(int r,int c,int v) { if (r<0||r>=9||c<0||c>=9) return; grid[r][c] = v; }

private:
    std::array<std::array<int,9>,9> grid{};
    std::array<std::array<int,9>,9> solution{};
    std::array<std::array<bool,9>,9> fixed{};
    bool solutionFilled = false;
    std::mt19937 rng{static_cast<unsigned>(std::chrono::high_resolution_clock::now().time_since_epoch().count())};

    bool fillBacktrack(int pos = 0) {
        if (pos >= 81) return true;
        int r = pos / 9, c = pos % 9;
        if (grid[r][c] != 0) return fillBacktrack(pos+1);
        std::array<int,9> vals;
        for (int i=0;i<9;++i) vals[i]=i+1;
        std::shuffle(vals.begin(), vals.end(), rng);
        for (int v : vals) {
            if (canPlaceQuiet(r,c,v)) {
                grid[r][c] = v;
                if (fillBacktrack(pos+1)) return true;
                grid[r][c] = 0;
            }
        }
        return false;
    }

    void fillFullBoard() {
        for (auto &row : grid) row.fill(0);
        // fill diagonal boxes first
        for (int k=0;k<3;++k) {
            std::array<int,9> vals;
            for (int i=0;i<9;++i) vals[i]=i+1;
            std::shuffle(vals.begin(), vals.end(), rng);
            int br=k*3, bc=k*3, idx=0;
            for (int i=0;i<3;++i) for (int j=0;j<3;++j) grid[br+i][bc+j]=vals[idx++];
        }
        if (!fillBacktrack(0)) fillFullBoard();
    }

    bool canPlaceQuiet(int r,int c,int v) const {
        if (v < 1 || v > 9) return false;
        for (int i = 0; i < 9; ++i) if (grid[r][i] == v) return false;
        for (int i = 0; i < 9; ++i) if (grid[i][c] == v) return false;
        int br=(r/3)*3, bc=(c/3)*3;
        for (int i=0;i<3;++i) for (int j=0;j<3;++j) if (grid[br+i][bc+j]==v) return false;
        return true;
    }

    int countSolutionsLimit(int limit=2) {
        int tmp[9][9];
        for (int r=0;r<9;++r) for (int c=0;c<9;++c) tmp[r][c] = grid[r][c];

        int solutions = 0;
        std::mt19937 locrng(static_cast<unsigned>(rng()));
        std::function<void()> dfs;
        auto findUnfilled = [&]() -> int {
            for (int i=0;i<81;++i) {
                int r=i/9,c=i%9;
                if (tmp[r][c]==0) return i;
            }
            return -1;
        };

        dfs = [&]() {
            if (solutions >= limit) return;
            int pos = findUnfilled();
            if (pos == -1) { ++solutions; return; }
            int r = pos/9, c = pos%9;
            std::array<int,9> vals; for (int i=0;i<9;++i) vals[i]=i+1;
            std::shuffle(vals.begin(), vals.end(), locrng);
            for (int v : vals) {
                bool ok = true;
                for (int i=0;i<9;++i) if (tmp[r][i] == v) { ok=false; break; }
                if (!ok) continue;
                for (int i=0;i<9;++i) if (tmp[i][c] == v) { ok=false; break; }
                if (!ok) continue;
                int br=(r/3)*3, bc=(c/3)*3;
                for (int i=0;i<3 && ok;++i) for (int j=0;j<3;++j) if (tmp[br+i][bc+j]==v) { ok=false; break; }
                if (!ok) continue;
                tmp[r][c]=v;
                dfs();
                tmp[r][c]=0;
                if (solutions>=limit) return;
            }
        };

        dfs();
        return solutions;
    }

    void removeUntilClues(int cluesTarget) {
        std::vector<int> cells(81); std::iota(cells.begin(), cells.end(), 0);
        std::shuffle(cells.begin(), cells.end(), rng);
        int currentClues = 81;
        for (int idx : cells) {
            if (currentClues <= cluesTarget) break;
            int r = idx/9, c = idx%9;
            int backup = grid[r][c];
            grid[r][c] = 0;
            int sols = countSolutionsLimit(2);
            if (sols != 1) grid[r][c] = backup;
            else --currentClues;
        }
    }

    static int difficultyClues(Difficulty d) {
        switch (d) {
            case Difficulty::Easy: return 40;
            case Difficulty::Medium: return 34;
            case Difficulty::Hard: return 28;
            default: return 34;
        }
    }
};

// ---------- Terminal raw mode ----------
class TermiosGuard {
public:
    TermiosGuard() { tcgetattr(STDIN_FILENO, &orig); termios raw = orig; raw.c_lflag &= ~(ECHO | ICANON); raw.c_cc[VMIN]=1; raw.c_cc[VTIME]=0; tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); }
    ~TermiosGuard() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig); }
private:
    termios orig{};
};

// Read a key (handles arrow escape sequences)
std::string readKey() {
    char c;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    if (n <= 0) return "";
    if (c == '\x1b') {
        // attempt to read two more bytes (typical arrow sequences)
        char seq[2] = {0,0};
        ssize_t n1 = read(STDIN_FILENO, &seq[0], 1);
        ssize_t n2 = read(STDIN_FILENO, &seq[1], 1);
        std::string s;
        s.push_back('\x1b');
        if (n1>0) s.push_back(seq[0]);
        if (n2>0) s.push_back(seq[1]);
        return s;
    }
    return std::string(1, c);
}

// ---------- SudokuGame: UI, loop, input ----------
class SudokuGame {
public:
    SudokuGame() : board(), cursorR(0), cursorC(0), diff(Difficulty::Easy) {
        board.generate(diff);
        while (!undo.empty()) undo.pop();
    }

    void run() {
        TermiosGuard tg;
        loop();
    }

private:
    SudokuBoard board;
    int cursorR, cursorC;
    Difficulty diff;
    std::stack<Move> undo;
    bool flashInvalid = false;
    std::chrono::steady_clock::time_point flashUntil;
    bool exited = false;

    void loop() {
        while (!exited) {
            clearScreen();
            renderHeader();
            renderBoard();
            renderFooter();
            if (flashInvalid && std::chrono::steady_clock::now() > flashUntil) flashInvalid = false;
            std::string k = readKey();
            if (k.empty()) continue;
            handleKey(k);
            if (board.isSolved()) {
                clearScreen();
                renderHeader();
                renderBoard();
                renderFooter();
                celebrate();
                if (!promptRestart()) { exited = true; break; }
                board.generate(diff);
                while (!undo.empty()) undo.pop();
                cursorR = cursorC = 0;
            }
        }
    }

    void handleKey(const std::string& key) {
        if (key == "\x1b[A") { if (cursorR>0) --cursorR; }
        else if (key == "\x1b[B") { if (cursorR<8) ++cursorR; }
        else if (key == "\x1b[C") { if (cursorC<8) ++cursorC; }
        else if (key == "\x1b[D") { if (cursorC>0) --cursorC; }
        else if (key == "q" || key == "Q") { exited = true; }
        else if (key == "u" || key == "U") undoOne();
        else if (key == "h" || key == "H") doHint();
        else if (key == "r" || key == "R") restart();
        else if (key == "d" || key == "D") changeDifficulty();
        else if (key == "\x7f" || key == "\b" || key == "0") clearCell();
        else if (key == ":") {
            std::string line; restoreAndReadLine(line); parseCoordinateInput(line);
        } else if (key.size()==1 && isdigit((unsigned char)key[0])) {
            int v = key[0]-'0';
            if (v >= 1 && v <= 9) placeNumber(v);
        }
    }

    void undoOne() {
        if (undo.empty()) { flash(); return; }
        Move m = undo.top(); undo.pop();
        board.set(m.r, m.c, m.prev);
    }

    void doHint() {
        auto mv = board.hintOne();
        if (!mv) { flash(); return; }
        undo.push(Move{mv->r, mv->c, 0});
    }

    void restart() {
        board.generate(diff);
        while (!undo.empty()) undo.pop();
        cursorR = cursorC = 0;
    }

    void changeDifficulty() {
        if (diff == Difficulty::Easy) diff = Difficulty::Medium;
        else if (diff == Difficulty::Medium) diff = Difficulty::Hard;
        else diff = Difficulty::Easy;
        restart();
    }

    void clearCell() {
        if (board.isFixed(cursorR,cursorC)) { flash(); return; }
        int prev = board.at(cursorR,cursorC);
        if (prev != 0) {
            board.set(cursorR,cursorC,0);
            undo.push(Move{cursorR,cursorC,prev});
        }
    }

    void placeNumber(int v) {
        if (board.isFixed(cursorR,cursorC)) { flash(); return; }
        int prev = board.at(cursorR,cursorC);
        bool ok = board.placeValue(cursorR,cursorC,v);
        if (!ok) flash();
        else undo.push(Move{cursorR,cursorC,prev});
    }

    void parseCoordinateInput(const std::string& line) {
        std::istringstream iss(line);
        std::vector<int> nums;
        char ch;
        while (iss >> ch) {
            if (std::isdigit((unsigned char)ch)) nums.push_back(ch - '0');
        }
        if (nums.size() >= 3) {
            int r = nums[0], c = nums[1], v = nums[2];
            if (r<1||r>9||c<1||c>9||v<0||v>9) { flash(); return; }
            int rr = r-1, cc = c-1;
            if (v==0) {
                if (board.isFixed(rr,cc)) { flash(); return; }
                int prev = board.at(rr,cc);
                board.set(rr,cc,0);
                undo.push(Move{rr,cc,prev});
            } else {
                if (board.isFixed(rr,cc)) { flash(); return; }
                int prev = board.at(rr,cc);
                bool ok = board.placeValue(rr,cc,v);
                if (!ok) flash(); else undo.push(Move{rr,cc,prev});
            }
            cursorR = rr; cursorC = cc;
        } else {
            flash();
        }
    }

    void restoreAndReadLine(std::string& out) {
        termios orig;
        tcgetattr(STDIN_FILENO, &orig);
        termios cooked = orig;
        cooked.c_lflag |= (ECHO | ICANON);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &cooked);
        std::cout << "\nEnter coordinate (e.g. 3 4 5), or 0 to clear: ";
        std::cout.flush();
        std::string line;
        std::getline(std::cin, line);
        out = line;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
    }

    void flash() {
        flashInvalid = true;
        flashUntil = std::chrono::steady_clock::now() + 350ms;
        std::cout << "\a";
    }

    void clearScreen() {
        std::cout << ansi::CLEAR_SCREEN;
    }

    void renderHeader() {
        std::cout << ansi::BOLD << ansi::FG_YELLOW << " Sudoku (Dark Mode) " << ansi::RESET;
        std::cout << "    Controls: Arrows move  1-9 place  0/Backspace clear  U undo  H hint  : coord input\n";
        std::cout << "             D change difficulty  R restart  Q quit\n\n";
        std::cout << " Difficulty: ";
        if (diff==Difficulty::Easy) std::cout << ansi::FG_GREEN << "Easy" << ansi::RESET;
        else if (diff==Difficulty::Medium) std::cout << ansi::FG_YELLOW << "Medium" << ansi::RESET;
        else std::cout << ansi::FG_RED << "Hard" << ansi::RESET;
        std::cout << "\n\n";
    }

    void renderBoard() {
        std::cout << "  " << box::TL;
        for (int c=0;c<9;++c) {
            std::cout << box::H << box::H << box::H;
            if (c==8) std::cout << box::TR;
            else if ((c+1)%3==0) std::cout << box::T;
            else std::cout << box::I_H;
        }
        std::cout << "\n";
        for (int r=0;r<9;++r) {
            std::cout << (r+1) << " " << box::V;
            for (int c=0;c<9;++c) {
                bool isCursor = (r==cursorR && c==cursorC);
                int val = board.at(r,c);
                bool isFixed = board.isFixed(r,c);
                std::ostringstream ss;
                if (isCursor) ss << ansi::BG_BLUE;
                if (val != 0) {
                    if (isFixed) ss << ansi::FG_CYAN << ansi::BOLD;
                    else {
                        if (flashInvalid && !board.canPlace(r,c,val)) ss << ansi::FG_RED << ansi::BOLD;
                        else ss << ansi::FG_WHITE << ansi::BOLD;
                    }
                } else {
                    ss << ansi::FG_WHITE;
                }
                ss << " " << (val==0 ? ' ' : char('0'+val)) << " ";
                ss << ansi::RESET;
                std::cout << ss.str();
                if (c==8) std::cout << box::V;
                else if ((c+1)%3==0) std::cout << box::V;
                else std::cout << box::I_V;
            }
            std::cout << "\n";
            if (r==8) {
                std::cout << "  " << box::BL;
                for (int c=0;c<9;++c) {
                    std::cout << box::H << box::H << box::H;
                    if (c==8) std::cout << box::BR;
                    else if ((c+1)%3==0) std::cout << box::B;
                    else std::cout << box::I_H;
                }
                std::cout << "\n";
            } else if ((r+1)%3==0) {
                std::cout << "  " << box::L;
                for (int c=0;c<9;++c) {
                    std::cout << box::H << box::H << box::H;
                    if (c==8) std::cout << box::R;
                    else if ((c+1)%3==0) std::cout << box::C;
                    else std::cout << box::I_H;
                }
                std::cout << "\n";
            } else {
                std::cout << "  " << box::I_L;
                for (int c=0;c<9;++c) {
                    std::cout << box::I_H << box::I_H << box::I_H;
                    if (c==8) std::cout << box::I_R;
                    else std::cout << box::I_C;
                }
                std::cout << "\n";
            }
        }
    }

    void renderFooter() {
        std::cout << "\n Cursor: R" << (cursorR+1) << " C" << (cursorC+1) << "    ";
        std::cout << "Undos: " << undo.size() << "    ";
        if (flashInvalid) std::cout << ansi::FG_RED << "Invalid Move!" << ansi::RESET;
        std::cout << "\n";
        std::cout << " Enter ':' for coordinate input (e.g. : 3 4 5 )\n";
    }

    void celebrate() {
        std::vector<std::string> art = {
            "  ____   _   _  _   _  _   _ ",
            " / ___| | | | || \\ | || \\ | |",
            "| |     | | | ||  \\| ||  \\| |",
            "| |___  | |_| || |\\  || |\\  |",
            " \\____|  \\___/ |_| \\_||_| \\_|"
        };
        using namespace ansi;
        for (int k=0;k<8;++k) {
            clearScreen();
            const char* colors[6] = {FG_RED, FG_YELLOW, FG_GREEN, FG_CYAN, FG_WHITE, FG_YELLOW};
            int ci = k % 6;
            std::cout << colors[ci] << BOLD;
            for (auto &line : art) std::cout << "   " << line << "\n";
            std::cout << RESET << "\n";
            std::cout << colors[ci] << BOLD << "Congratulations! You solved the puzzle!\n" << RESET;
            std::cout << " Press R to play again, or Q to quit.\n";
            std::cout.flush();
            ansi::sleep_ms(160);
        }
    }

    bool promptRestart() {
        while (true) {
            std::string k = readKey();
            if (k == "r" || k == "R") return true;
            if (k == "q" || k == "Q") return false;
        }
    }
};

// ---------- main ----------
int main() {
    try {
        SudokuGame game;
        game.run();
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}