#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <cstdlib>
#include <algorithm>

void displayHangman(int attempts) {
    const std::vector<std::string> hangmanStages = {
        "  +---+\n      |\n      |\n      |\n     ===",
        "  +---+\n  O   |\n      |\n      |\n     ===",
        "  +---+\n  O   |\n  |   |\n      |\n     ===",
        "  +---+\n  O   |\n /|   |\n      |\n     ===",
        "  +---+\n  O   |\n /|\\  |\n      |\n     ===",
        "  +---+\n  O   |\n /|\\  |\n /    |\n     ===",
        "  +---+\n  O   |\n /|\\  |\n / \\  |\n     ==="
    };
    std::cout << hangmanStages[attempts] << "\n";
}

std::string getRandomWord() {
    std::vector<std::string> words = {
    "apple", "banana", "orange", "grape", "peach", 
    "table", "chair", "pencil", "paper", "school", 
    "window", "garden", "flower", "butter", "honey", 
    "coffee", "bottle", "pillow", "blanket", "mirror", 
    "family", "friend", "travel", "ticket", "summer", 
    "winter", "spring", "autumn", "morning", "evening", 
    "animal", "planet", "ocean", "forest", "desert", 
    "bridge", "street", "village", "castle", "market", 
    "doctor", "teacher", "artist", "driver", "farmer", 
    "puzzle", "garden", "shadow", "pocket", "rocket"
};
    srand(time(0));
    return words[rand() % words.size()];
}

void playGame() {
    std::string word = getRandomWord();
    std::string guessedWord(word.size(), '_');
    int maxAttempts = 6;
    int attempts = 0;
    std::vector<char> guessedLetters;

    std::cout << "Welcome to Hangman!\n";
    while (attempts < maxAttempts && guessedWord != word) {
        std::cout << "\nWord: " << guessedWord << "\n";
        displayHangman(attempts);
        std::cout << "Guessed letters: ";
        for (char c : guessedLetters) std::cout << c << " ";
        std::cout << "\n";

        std::cout << "Enter a letter: ";
        char guess;
        std::cin >> guess;
        guess = tolower(guess);

        if (std::find(guessedLetters.begin(), guessedLetters.end(), guess) != guessedLetters.end()) {
            std::cout << "You already guessed that letter. Try again.\n";
            continue;
        }

        guessedLetters.push_back(guess);

        if (word.find(guess) != std::string::npos) {
            std::cout << "Correct guess!\n";
            for (size_t i = 0; i < word.size(); ++i) {
                if (word[i] == guess) guessedWord[i] = guess;
            }
        } else {
            std::cout << "Wrong guess!\n";
            attempts++;
        }
    }

    if (guessedWord == word) {
        std::cout << "\nCongratulations! You guessed the word: " << word << "\n";
    } else {
        displayHangman(attempts);
        std::cout << "\nGame Over! The word was: " << word << "\n";
    }
}

int main() {
    playGame();
    return 0;
}