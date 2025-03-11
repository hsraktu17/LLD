#include<bits/stdc++.h>

using namespace std;

class TicTacToe{
private:
    vector<vector<char>> board;
    string playerX, playerY;
    char currentPlayer;
    int moves;

    void printBoard(){
        cout<<"------------------------------"<<endl;
        for(int i = 0; i< board.size(); i++){
            for(int j = 0;j < board[i].size(); j++){
                cout<<board[i][j] <<" ";
            }
            cout<<endl;
        }
        cout<<"------------------------------"<<endl;
    }

    bool isValid(int row, int col){
        return row >= 0 && row < 3 && col >= 0 && col < 3 && board[row][col] == '-';
    }

    bool checkWin(){
        for(int i = 0;i < 3; i++){
            if(board[i][0] == currentPlayer && board[i][1] == currentPlayer && board[i][2] == currentPlayer){
                return true;
            }
            if(board[0][i] == currentPlayer && board[1][i] == currentPlayer && board[2][i] == currentPlayer){
                return true;
            }
        }
        if(board[0][0] == currentPlayer && board[1][1] == currentPlayer && board[2][2] == currentPlayer){
            return true;
        }
        if(board[0][2] == currentPlayer && board[1][1] == currentPlayer && board[2][0] == currentPlayer){
            return true;
        }
        return false;
    }

public: 
    TicTacToe(string pX, string pY){
        playerX = pX;
        playerY = pY;
        currentPlayer = 'X';
        moves = 0;
        board = vector<vector<char>>(3, vector<char>(3,'-'));
        printBoard();
    }

    void playGame(){
        string input;
        while(true){
            cin>>input;
            if(input == "exit"){
                break;
            }

            int row, col;
            row = stoi(input) -1;
            cin>> col;
            col = col -1;
            if(!isValid(row, col)){
                cout<<"valid moves"<<endl;
                continue;
            }

            board[row][col] = currentPlayer;
            moves++;
            printBoard();

            if(checkWin()){
                cout<< (currentPlayer == 'X' ? playerX : playerY) <<"won the game"<<endl;
                return;
            }

            if(moves == 9){
                cout<<"Game over"<<endl;
                return;
            }
            currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
        }
    }
};

int main(){
    char playerX, playerY;
    string nameX, nameY;
    
    cout<<"Enter symbol and name for player 1"<<endl;
    cin>> playerX >> nameX;

    cout<<"Enter symbol and name for player 2"<<endl;
    cin>> playerY >> nameY;

    TicTacToe game(nameX, nameY);
    game.playGame();
    
    return 0;
}
