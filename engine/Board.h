#pragma once
#include <iostream>
#include "Types.h"
#include <vector>
#include <cstdint>
#include "move.h"
using namespace std;

enum class SideToMove{
    white,black
};
struct Castling{
    bool wk=true,wq=true,bk=true,bq=true;
};
struct Undo
{
    Move move;
    char captured = '.';
    Castling castling;
    Square enPassantSquare{-1, -1};

    int halfmove = 0;
    int fullmove = 1;

    SideToMove sideToMove = SideToMove::white;
    std::uint64_t zobristHash = 0;
};
class Board{
    private:
    char position[8][8];
    SideToMove sidetomove;
    int halfmove,fullmove;
    Castling castling;
    Square enPassantSquare;

    vector<Undo> history;
    vector<string> positionKeys;
    vector<std::uint64_t> hashHistory;
    Square kingSquares[2]{{-1, -1}, {-1, -1}};
    std::uint64_t zobristHash = 0;

    // for makemove
    char promotedPiece(Colour colour, char typeChar) const;
    void applyCastlingRook(const Move& move);
    void removeEnPassantVictim(const Move& move);
    void updateEnPassantSquare(const Move& move);
    void updateClocks(char piece, bool wasCapture);
    void updateCastlingRights(const Move& move, char piece);
    void clearRightForSquare(const Square& square);

    //for unodMove()
    void undoCastlingRook(const Move& move);
    void restoreFromUndo(const Undo& record);
    public:
    // Constructor
    Board();
    //chks if same board
    bool isSameAs(const Board& other) const;
    //Board Chg
    void makeMove(const Move& move);

    //apply undo
    bool undoMove();
    // Board state
    void clear();
    bool loadFEN(
        const string& fen,
        string* error = nullptr
    );
    void loadStartPosition();

    // Piece access
    char getPiece(int r, int c) const;
    void setPiece(int r, int c, char piece);

    // Getters
    SideToMove getSideToMove() const;
    Castling getCastlingRights() const;
    Square getEnPassantSquare() const;

    int getHalfMove() const;

    int getFullMove() const;

    int historySize() const;

    Square getKingSquare(Colour colour) const;
    std::uint64_t getZobristHash() const;
    std::uint64_t recomputeZobristHash() const;
    const vector<std::uint64_t>& getHashHistory() const;

    const vector<string>& getPositionKeys() const;

    // Output
    string toFEN() const;

    void print() const;

};
string positionKey(const Board& board);
