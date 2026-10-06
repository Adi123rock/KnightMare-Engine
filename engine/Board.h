#pragma once
#include <iostream>
#include "Types.h"
using namespace std;

enum class SideToMove{
    white,black
};
struct Castling{
    bool wk=true,wq=true,bk=true,bq=true;
};
class Board{
    private:
    char position[8][8];
    SideToMove sidetomove;
    int halfmove,fullmove;
    Castling castling;
    Square enPassantSquare;
    
    public:
    public:
    // Constructor
    Board();

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

    // Output
    string toFEN() const;

    void print() const;

};