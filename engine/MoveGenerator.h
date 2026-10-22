#pragma once
#include <vector>
#include "Board.h"
#include "move.h"
using namespace std;

namespace MoveGenerator
{
    bool isEnemy(char piece, Colour colour);
    bool isFriendly(char piece, Colour colour);

    void addMove(vector<Move>& list,const Square& from,const Square& to,
        int flags = 0,char promotion = '.',char captured = '.');
    
    void generateKnightMoves(const Board& board,const Square& from,vector<Move>& list);

    void generateSlidingMoves(const Board& board,const Square& from,
    const int directions[][2],int directionCount,vector<Move>& list);

    void generateBishopMoves(const Board& board,const Square& from,vector<Move>& list);

    void generateRookMoves(const Board& board,const Square& from,vector<Move>& list);

    void generateQueenMoves(const Board& board,const Square& from,vector<Move>& list);

    void generateKingMoves(const Board& board,const Square& from,vector<Move>& list);

    //for pawn move generation
    int pawnDirection(Colour colour);
    int pawnStartRow(Colour colour);
    int pawnPromotionRow(Colour colour);

    void addPromotionMoves(vector<Move>& list,const Square& from,
        const Square& to,bool isCapture,char captured);

    void generatePawnMoves(const Board& board,const Square& from,vector<Move>& list);

    void generateEnPassant(const Board& board,const Square& from,vector<Move>& list);

    void generateCastlingMoves(const Board& board, vector<Move>& list);
    bool squaresEmpty(const Board& board, int row, int colFrom, int colTo);

    void generateMovesForPiece(const Board& board,const Square& from,vector<Move>& list);

    vector<Move> generateAllMoves(const Board& board);
}