
#include "legal.h"
#include "attacks.h"
#include "MoveGenerator.h"

#include <algorithm>
//candiadate is our user
vector<Move> generateLegalMoves(Board& board)
{
    // generate pseudo-legal moves.
    vector<Move> pseudoMoves = MoveGenerator::generateAllMoves(board);
    vector<Move> legalMoves;

    // Save the mover BEFORE makeMove switches sides.
    Colour mover =
        (board.getSideToMove() == SideToMove::white)? Colour::white: Colour::black;

    // Colour enemy = opposite(mover);

    for (const Move& move : pseudoMoves)
    {
        // Castling cannot start in check or cross an attacked square.
        if (hasFlag(move, MOVE_KING_CASTLE) || hasFlag(move, MOVE_QUEEN_CASTLE))
        {
            if (!isCastlingLegal(board, move)) continue;
        }
        // Try the candidate move.
        board.makeMove(move);

        // Check the original mover's king, not the new side to move.
        bool leavesKingInCheck = isInCheck(board, mover);

        // Always undo after making a candidate move.
        bool undone = board.undoMove();

        // Do not silently accept a move if undo unexpectedly fails.
        if (!undone) return {};

        if (!leavesKingInCheck) legalMoves.push_back(move);
    }

    return legalMoves;
}


bool isLegalMove(Board& board, const Move& move)
{
    vector<Move> legalMoves = generateLegalMoves(board);
    for (const Move& candidate : legalMoves)
    {
        if (candidate.from.r == move.from.r &&
            candidate.from.c == move.from.c && candidate.to.r == move.to.r &&
            candidate.to.c == move.to.c && candidate.promotion == move.promotion)
        {
            return true;
        }
    }
    return false;
}



bool isCastlingLegal(const Board& board, const Move& move)
{
    bool kingSide = hasFlag(move, MOVE_KING_CASTLE);
    bool queenSide = hasFlag(move, MOVE_QUEEN_CASTLE);

    // This function only handles castling moves.
    if (!kingSide && !queenSide) return false;

    if (!inBounds(move.from.r, move.from.c) ||
        !inBounds(move.to.r, move.to.c))
    {
        return false;
    }

    // Castling must move the king two files on the same rank.
    if (move.from.r != move.to.r ||
        move.to.c - move.from.c != (kingSide ? 2 : -2)) return false;

    Colour mover =
        (board.getSideToMove() == SideToMove::white)? Colour::white: Colour::black;

    Colour enemy = opposite(mover);

    // The king must not currently be in check.
    if (isInCheck(board, mover)) return false;

    // The square the king passes over.
    Square transit{move.from.r,(move.from.c + move.to.c) / 2};

    if (isSquareAttacked(board, transit, enemy)) return false;

    // Explicitly check the king's destination square.
    if (isSquareAttacked(board, move.to, enemy)) return false;

    return true;
}
