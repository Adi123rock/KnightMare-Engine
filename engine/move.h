#pragma once
using namespace std;
#include "Types.h"

// Move Flags

constexpr int MOVE_CAPTURE      = 1 << 0;
constexpr int MOVE_DOUBLE_PUSH  = 1 << 1;
constexpr int MOVE_PROMOTION    = 1 << 2;
constexpr int MOVE_KING_CASTLE = 1 << 3;
constexpr int MOVE_QUEEN_CASTLE = 1 << 4;
constexpr int MOVE_EN_PASSANT  = 1 << 5;


// Move

struct Move
{
    Square from{};
    Square to{};

    char promotion = '.';
    char captured = '.';

    int flags = 0;
};

//Move helpers
inline string moveToString(const Move& move)
{
    string result;

    result += squareToName(move.from);
    result += squareToName(move.to);

    if (move.promotion != '.')
        result += move.promotion;

    return result;
}

inline bool stringToMove(const string& text, Move& outMove)
{
    // Move must be either:
    // e2e4       -> 4 characters
    // e7e8q      -> 5 characters
    if (text.length() != 4 && text.length() != 5)
        return false;

    string fromName = text.substr(0, 2);
    string toName   = text.substr(2, 2);

    Square from, to;

    if (!nameToSquare(fromName, from))
        return false;

    if (!nameToSquare(toName, to))
        return false;

    char promotion = '.';

    if (text.length() == 5)
    {
        promotion = text[4];
        if (promotion != 'q' && promotion != 'r' && promotion != 'b' && promotion != 'n')
        {
            return false;
        }
    }

    // Create the parsed move
    outMove.from = from;
    outMove.to = to;
    outMove.promotion = promotion;
    outMove.flags = 0;
    outMove.captured = '.';

    return true;
}

// Flag Helpers

inline bool hasFlag(const Move& move, int flag)
{
    return (move.flags & flag) != 0;
}


inline bool isCapture(const Move& move)
{
    return hasFlag(move, MOVE_CAPTURE);
}


inline bool isPromotion(const Move& move)
{
    return hasFlag(move, MOVE_PROMOTION);
}


inline bool isEnPassant(const Move& move)
{
    return hasFlag(move, MOVE_EN_PASSANT);
}

inline bool isCastle(const Move& move)
{
    return hasFlag(move, MOVE_KING_CASTLE) || hasFlag(move, MOVE_QUEEN_CASTLE);
}

inline bool isDoublePush(const Move& move)
{
    return hasFlag(move, MOVE_DOUBLE_PUSH);
}

// Move Creation Helper

inline Move makeMoveObject(
    Square from,Square to,int flags = 0,char promotion = '.')
{
    Move move;

    move.from = from;
    move.to = to;
    move.flags = flags;
    move.promotion = promotion;

    return move;
}