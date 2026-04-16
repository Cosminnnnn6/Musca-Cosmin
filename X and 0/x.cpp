// cpp
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <random>
#include <chrono>
#include <cctype>
#include <limits>

// Represents current state of the game
enum class GameState {
    InProgress,
    Win,
    Draw
};

// Board class: encapsulates the 3x3 board and related operations
class Board {
public:
    Board() { reset(); }

    // Reset board to initial empty state
    void reset() {
        cells.assign(9, ' ');
    }

    // Attempt to place marker ('X' or 'O') at position [1..9]; returns true on success
    bool placeMarker(int position, char marker) {
        if (position < 1 || position > 9) return false;
        int idx = position - 1;
        if (cells[idx] != ' ') return false;
        cells[idx] = marker;
        return true;
    }

    // Check if a position is free
    bool isEmptyAt(int position) const {
        if (position < 1 || position > 9) return false;
        return cells[position - 1] == ' ';
    }

    // Check for a winner. Returns marker ('X'/'O') if winner exists, otherwise ' '.
    char checkWinner() const {
        static const int wins[8][3] = {
            {0,1,2}, {3,4,5}, {6,7,8}, // rows
            {0,3,6}, {1,4,7}, {2,5,8}, // cols
            {0,4,8}, {2,4,6}           // diagonals
        };

        for (auto &w : wins) {
            char a = cells[w[0]];
            if (a != ' ' && a == cells[w[1]] && a == cells[w[2]]) {
                return a;
            }
        }
        return ' ';
    }

    // Returns true if board is full (no empty cells)
    bool isFull() const {
        return std::none_of(cells.begin(), cells.end(), [](char c){ return c == ' '; });
    }

    // Visual display of the board; empty cells show their position number (1..9)
    void display() const {
        auto cellDisplay = [this](int idx) -> char {
            return (cells[idx] == ' ') ? ('1' + idx) : cells[idx];
        };

        std::cout << "\n " << cellDisplay(0) << " | " << cellDisplay(1) << " | " << cellDisplay(2) << "\n";
        std::cout << "---+---+---\n";
        std::cout << " " << cellDisplay(3) << " | " << cellDisplay(4) << " | " << cellDisplay(5) << "\n";
        std::cout << "---+---+---\n";
        std::cout << " " << cellDisplay(6) << " | " << cellDisplay(7) << " | " << cellDisplay(8) << "\n\n";
    }

    // Get a list of empty positions (1..9)
    std::vector<int> emptyPositions() const {
        std::vector<int> result;
        for (int i = 0; i < 9; ++i) if (cells[i] == ' ') result.push_back(i + 1);
        return result;
    }

    // Get the marker at a given position (1..9)
    char at(int position) const {
        if (position < 1 || position > 9) return ' ';
        return cells[position - 1];
    }

private:
    std::vector<char> cells; // 9 cells row-major
};

// Abstract Player class
class Player {
public:
    Player(std::string name, char marker) : name_(std::move(name)), marker_(marker) {}
    virtual ~Player() = default;

    char marker() const noexcept { return marker_; }
    const std::string& name() const noexcept { return name_; }

    // Get next move (position 1..9). Implemented by derived classes.
    virtual int getMove(const Board& board) = 0;

protected:
    std::string name_;
    char marker_;
};

// Human player: reads validated input from console
class HumanPlayer : public Player {
public:
    HumanPlayer(const std::string& name, char marker) : Player(name, marker) {}

    int getMove(const Board& board) override {
        while (true) {
            std::cout << name_ << " (" << marker_ << "), enter position (1-9): ";
            std::string line;
            if (!std::getline(std::cin, line)) {
                // EOF or input error; attempt to clear and continue
                std::cin.clear();
                continue;
            }
            // Trim whitespace
            auto firstNonSpace = line.find_first_not_of(" \t\r\n");
            if (firstNonSpace == std::string::npos) continue;
            char ch = line[firstNonSpace];
            if (!std::isdigit(static_cast<unsigned char>(ch))) {
                std::cout << "Invalid input. Please enter a number between 1 and 9.\n";
                continue;
            }
            int pos = ch - '0';
            if (pos < 1 || pos > 9) {
                std::cout << "Position out of range. Choose 1-9.\n";
                continue;
            }
            if (!board.isEmptyAt(pos)) {
                std::cout << "Position " << pos << " is already occupied. Choose another.\n";
                continue;
            }
            return pos;
        }
    }
};

// Simple AI player: attempts to win or block; otherwise picks center, corner, or random
class AIPlayer : public Player {
public:
    AIPlayer(const std::string& name, char marker)
        : Player(name, marker),
          rng_(static_cast<unsigned>(std::chrono::high_resolution_clock::now().time_since_epoch().count()))
    {}

    int getMove(const Board& board) override {
        std::cout << name_ << " (" << marker_ << ") is thinking...\n";

        char opponent = (marker_ == 'X') ? 'O' : 'X';
        auto empties = board.emptyPositions();

        // 1) Win if possible
        for (int pos : empties) {
            if (wouldWin(board, pos, marker_)) return pos;
        }
        // 2) Block opponent win
        for (int pos : empties) {
            if (wouldWin(board, pos, opponent)) return pos;
        }
        // 3) Take center if free
        if (board.isEmptyAt(5)) return 5;
        // 4) Take a corner if available (1,3,7,9)
        std::vector<int> corners;
        for (int c : {1,3,7,9}) if (board.isEmptyAt(c)) corners.push_back(c);
        if (!corners.empty()) {
            std::uniform_int_distribution<int> dist(0, static_cast<int>(corners.size()) - 1);
            return corners[dist(rng_)];
        }
        // 5) Take any side or random empty
        if (!empties.empty()) {
            std::uniform_int_distribution<int> dist(0, static_cast<int>(empties.size()) - 1);
            return empties[dist(rng_)];
        }
        return 1; // fallback (should not reach here)
    }

private:
    // Simulate placing marker at pos and see if it yields a win
    bool wouldWin(const Board& board, int pos, char marker) {
        // Create temporary copy to test
        Board temp = board;
        if (!temp.placeMarker(pos, marker)) return false;
        return temp.checkWinner() == marker;
    }

    std::mt19937 rng_;
};

// Game class: orchestrates gameplay, turns, state checks, and replay
class Game {
public:
    Game() : board_(), currentIndex_(0), state_(GameState::InProgress) {}

    // Start the main loop: choose mode and run games until user quits
    void run() {
        std::cout << "Welcome to Tic-Tac-Toe!\n";
        while (true) {
            chooseMode();
            playSingleGame();
            if (!promptReplay()) break;
            board_.reset();
        }
        std::cout << "Thanks for playing!\n";
    }

private:
    Board board_;
    std::vector<std::unique_ptr<Player>> players_;
    int currentIndex_;
    GameState state_;

    // Ask user to pick mode (2-player or vs AI) and set up players
    void chooseMode() {
        players_.clear();
        while (true) {
            std::cout << "Select mode:\n";
            std::cout << "1) Two-player (Human vs Human)\n";
            std::cout << "2) Play vs Computer (Human vs AI)\n";
            std::cout << "Choose 1 or 2: ";
            std::string line;
            std::getline(std::cin, line);
            if (line.empty()) continue;
            char choice = line[0];
            if (choice == '1') {
                std::string name1, name2;
                std::cout << "Enter name for Player 1 (X): ";
                std::getline(std::cin, name1);
                if (name1.empty()) name1 = "Player 1";
                std::cout << "Enter name for Player 2 (O): ";
                std::getline(std::cin, name2);
                if (name2.empty()) name2 = "Player 2";
                players_.push_back(std::make_unique<HumanPlayer>(name1, 'X'));
                players_.push_back(std::make_unique<HumanPlayer>(name2, 'O'));
                break;
            } else if (choice == '2') {
                std::string name;
                std::cout << "Enter your name (X): ";
                std::getline(std::cin, name);
                if (name.empty()) name = "Player";
                players_.push_back(std::make_unique<HumanPlayer>(name, 'X'));
                players_.push_back(std::make_unique<AIPlayer>("Computer", 'O'));
                break;
            } else {
                std::cout << "Invalid selection. Please choose 1 or 2.\n";
            }
        }
    }

    // Play one full game (single round), handling turn alternation and win/draw detection
    void playSingleGame() {
        board_.reset();
        state_ = GameState::InProgress;
        currentIndex_ = 0; // Player 0 (X) starts

        // Continue until win or draw
        while (state_ == GameState::InProgress) {
            board_.display();
            Player& current = *players_[currentIndex_];
            std::cout << "Turn: " << current.name() << " [" << current.marker() << "]\n";

            int move = current.getMove(board_);
            // Place move (human input validated; AI also ensures valid move)
            bool placed = board_.placeMarker(move, current.marker());
            if (!placed) {
                // Should not happen for validated moves, but handle gracefully
                std::cout << "Failed to place marker at position " << move << ". Try again.\n";
                continue;
            }

            // Check for win
            char winner = board_.checkWinner();
            if (winner == current.marker()) {
                board_.display();
                std::cout << "Congratulations! " << current.name() << " (" << current.marker() << ") wins!\n";
                state_ = GameState::Win;
                break;
            }

            // Check for draw
            if (board_.isFull()) {
                board_.display();
                std::cout << "The game is a draw.\n";
                state_ = GameState::Draw;
                break;
            }

            // Next player's turn
            currentIndex_ = (currentIndex_ + 1) % static_cast<int>(players_.size());
        }
    }

    // Ask the user if they want to play again
    bool promptReplay() {
        while (true) {
            std::cout << "Play again? (y/n): ";
            std::string line;
            std::getline(std::cin, line);
            if (line.empty()) continue;
            char ch = std::tolower(static_cast<unsigned char>(line[0]));
            if (ch == 'y') return true;
            if (ch == 'n') return false;
            std::cout << "Please enter 'y' or 'n'.\n";
        }
    }
};

// Entry point
int main() {
    // Use C++ locale-independent character behavior for tolower via unsigned char conversions above when used
    Game game;
    game.run();
    return 0;
}