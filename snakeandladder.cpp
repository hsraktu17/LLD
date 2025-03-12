#include <iostream>
#include <vector>
#include <unordered_map>
#include <cstdlib>
#include <ctime>
using namespace std;

// Player class
class Player {
public:
    string name;
    int position;

    Player(string name) {
        this->name = name;
        this->position = 0;
    }

    void move(int steps) {
        position += steps;
    }

    string getName() {
        return name;
    }

    int getPosition() {
        return position;
    }
};

// Dice class
class Dice {
public:
    int roll() {
        return rand() % 6 + 1; // Simulating a 6-sided dice
    }
};

// Board class
class Board {
private:
    int size;
    unordered_map<int, int> snakes;
    unordered_map<int, int> ladders;

public:
    Board(int size) {
        this->size = size;
    }

    void addSnake(int head, int tail) {
        snakes[head] = tail;
    }

    void addLadder(int start, int end) {
        ladders[start] = end;
    }

    int getNewPosition(int position) {
        if (snakes.find(position) != snakes.end()) {
            cout << "Oops! Snake at " << position << " → " << snakes[position] << endl;
            return snakes[position];
        }
        if (ladders.find(position) != ladders.end()) {
            cout << "Yay! Ladder at " << position << " → " << ladders[position] << endl;
            return ladders[position];
        }
        return position;
    }

    bool isWinningPosition(int position) {
        return position >= size;
    }
};

// Game class
class Game {
private:
    vector<Player> players;
    Board board;
    Dice dice;

public:
    Game(int boardSize, vector<string> playerNames) : board(boardSize) {
        for (string name : playerNames) {
            players.push_back(Player(name));
        }
        srand(time(0)); // Seed random number generator
    }

    void setupBoard() {
        board.addSnake(17, 7);
        board.addSnake(54, 34);
        board.addSnake(62, 19);
        board.addSnake(98, 79);

        board.addLadder(3, 22);
        board.addLadder(5, 8);
        board.addLadder(20, 29);
        board.addLadder(27, 77);
        board.addLadder(39, 58);
        board.addLadder(70, 90);
    }

    void startGame() {
        bool winnerFound = false;
        while (!winnerFound) {
            for (auto& player : players) {
                int rollValue = dice.roll();
                cout << player.getName() << " rolls a " << rollValue << endl;

                int newPos = player.getPosition() + rollValue;
                if (newPos > 100) {
                    cout << player.getName() << " stays at " << player.getPosition() << endl;
                    continue;
                }

                newPos = board.getNewPosition(newPos);
                player.move(newPos - player.getPosition());

                cout << player.getName() << " moves to " << player.getPosition() << endl;

                if (board.isWinningPosition(player.getPosition())) {
                    cout << player.getName() << " wins!" << endl;
                    winnerFound = true;
                    break;
                }
            }
        }
    }
};

// Main function
int main() {
    vector<string> playerNames = {"Alice", "Bob"};
    Game game(100, playerNames);
    game.setupBoard();
    game.startGame();
    return 0;
}
