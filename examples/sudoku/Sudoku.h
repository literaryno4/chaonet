//
// Created by chao on 2022/5/24.
//

#ifndef CHAONET_SUDOKU_H
#define CHAONET_SUDOKU_H

#include <iostream>
#include <vector>
#include <string>

using std::vector;
using std::string;

namespace chaonet {

class Sudoku {
   private:
    vector<vector<char>> board_{9, vector<char>(9, '0')};
    vector<vector<char>> rows_{9, vector<char>(9, '0')};
    vector<vector<char>> cols_{9, vector<char>(9, '0')};
    vector<vector<char>> blocks_{9, vector<char>(9, '0')};
    bool solved_ = false;

   public:
    Sudoku() = default;
    Sudoku(const string& s) {
        update(s);
    }

    void update(const string& s) {
        if (s.size() < 81) {
            std::cout << "not valid sudoku string!";
            return;
        }
        solved_ = false;
        int si = 0;
        for (int i = 0; i < 9; ++i) {
            for (int j = 0; j < 9; ++j) {
                board_[i][j] = s[si++];
                if (board_[i][j] == '0') {
                    board_[i][j] = '.';
                }
            }
        }
    }

    string result() {
        if (!solved_) {
            std::cout << "not solved yet!";
            return "";
        }
        string s(81, '0');
        int si = 0;
        for (int i = 0; i < 9; ++i) {
            for (int j = 0; j < 9; ++j) {
                 s[si++] = board_[i][j];
            }
        }
        return s;
    }

   public:
    vector<vector<char>>& board() {
        return board_;
    }

    void solveSudoku() {
        if (solved_) {
            return;
        }
        rows_.assign(9, vector<char>(9, '0'));
        cols_.assign(9, vector<char>(9, '0'));
        blocks_.assign(9, vector<char>(9, '0'));
        if (!isValidSudoku(board_)) {
            std::cout << "not valid sudoku, no solution!";
            return;
        }
        traceback(board_, 0, 0);
    }

   private:
    bool isValidSudoku(vector<vector<char>>& board) {
        int length = board.size();
        for (int i = 0; i < length; ++i) {
            for (int j = 0; j < length; ++j) {
                char c = board[i][j];
                if (c != '.') {
                    int idx = c - '0' - 1;
                    int bidx = (i / 3) * 3 + j / 3;
                    if (rows_[i][idx] != '0' || cols_[j][idx] != '0' || blocks_[bidx][idx] != '0') {
                        return false;
                    } else {
                        rows_[i][idx] = 'v';
                        cols_[j][idx] = 'v';
                        blocks_[bidx][idx] = 'v';
                    }
                }
            }
        }
        return true;

    }
    bool curEleValid(vector<vector<char>>& board, int i, int j, int k) {
        int idx = k - 1;
        int bidx = (i / 3) * 3 + j / 3;
        if (rows_[i][idx] != '0' || cols_[j][idx] != '0' || blocks_[bidx][idx] != '0') {
            return false;
        }
        return true;
    }

    void updateRecord(int i, int j, int k) {
        int idx = k - 1;
        int bidx = (i / 3) * 3 + j / 3;
        rows_[i][idx] = 'v';
        cols_[j][idx] = 'v';
        blocks_[bidx][idx] = 'v';
    }

    void cancelRecord(int i, int j, int k) {
        int idx = k - 1;
        int bidx = (i / 3) * 3 + j / 3;
        rows_[i][idx] = '0';
        cols_[j][idx] = '0';
        blocks_[bidx][idx] = '0';
    }
    void traceback(vector<vector<char>>& board, int i, int j) {
        if (solved_) {
            return;
        }
        if (i == 9) {
            solved_ = true;
            return;
        }
        if (board[i][j] != '.') {
            if (j < 8) {
                traceback(board, i, j + 1);
            } else {
                traceback(board, i + 1, 0);
            }
            return;
        }
        for (int k = 1; k <= 9 && !solved_; ++k) {
            if (curEleValid(board, i, j, k)) {
                board[i][j] = k + '0';
                updateRecord(i, j, k);
                if (j < 8) {
                    traceback(board, i, j + 1);
                } else {
                    traceback(board, i + 1, 0);
                }
                if (solved_) {
                    return;
                }
                board[i][j] = '.';
                cancelRecord(i, j, k);
            }
        }
    }

};

}

#endif  // CHAONET_SUDOKU_H
