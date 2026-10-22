// Plain test-only minimax reference. This file deliberately does not include
// or call the production search or move-ordering code.

#include "minimax_reference.h"

#include "../engine/evaluation.h"
#include "../engine/legal.h"
#include "../engine/attacks.h"
#include "../engine/rules.h"
#include "../engine2/search_types.h"

#include <algorithm>
#include <vector>

namespace
{
int toWhite(int sideToMoveScore, SideToMove side)
{
    return side == SideToMove::white ? sideToMoveScore : -sideToMoveScore;
}

bool referenceRepetitionSeen(const Board& board)
{
    const auto& keys = board.getPositionKeys();
    if (keys.size() < 3) return false;

    const std::string current = positionKey(board);
    const int last = static_cast<int>(keys.size()) - 1;
    const int limit = board.getHalfMove();
    for (int i = last - 2, back = 2; i >= 0 && back <= limit;
         i -= 2, back += 2)
    {
        if (keys[i] == current) return true;
    }
    return false;
}

int terminalWhite(const Board& board, bool inCheck, int ply)
{
    if (!inCheck) return 0;
    const int loss = MATE - ply;
    return board.getSideToMove() == SideToMove::white ? -loss : loss;
}

int minimaxNode(Board& board, int depth, int ply, bool maximize)
{
    // Match production alphaBeta's non-quiescent horizon semantics: a static
    // evaluation is returned as soon as depth reaches zero.
    if (depth <= 0) return whiteViewEval(board);

    std::vector<Move> moves = generateLegalMoves(board);
    const bool inCheck = isInCheck(
        board,
        board.getSideToMove() == SideToMove::white
            ? Colour::white : Colour::black);
    if (moves.empty()) return terminalWhite(board, inCheck, ply);

    // Draw rules match production at non-root interior nodes. Repetition is
    // independently detected from Board history instead of calling search.
    if (ply > 0 && (referenceRepetitionSeen(board) ||
                    isFiftyMoveDraw(board) || isInsufficientMaterial(board)))
        return 0;

    int best = maximize ? -INF : INF;
    for (const Move& move : moves)
    {
        board.makeMove(move);
        const int score = minimaxNode(board, depth - 1, ply + 1, !maximize);
        board.undoMove();
        best = maximize ? std::max(best, score) : std::min(best, score);
    }
    return best;
}
}

int whiteViewEval(const Board& board)
{
    // Production evaluate() is side-to-move-relative and includes tempo. Map
    // it into the reference's fixed White-positive perspective.
    return toWhite(evaluate(board), board.getSideToMove());
}

int minimaxSearch(Board& board, int depth, bool maximizing)
{
    return minimaxNode(board, depth, 0, maximizing);
}

MinimaxResult minimaxRoot(Board& board, int depth)
{
    MinimaxResult result;
    std::vector<Move> moves = generateLegalMoves(board);
    const bool inCheck = isInCheck(
        board,
        board.getSideToMove() == SideToMove::white
            ? Colour::white : Colour::black);
    if (moves.empty())
    {
        result.whiteScore = terminalWhite(board, inCheck, 0);
        return result;
    }

    if (referenceRepetitionSeen(board) || isFiftyMoveDraw(board) ||
        isInsufficientMaterial(board))
    {
        result.bestMove = moves.front();
        result.whiteScore = 0;
        return result;
    }

    const bool maximize = board.getSideToMove() == SideToMove::white;
    int best = maximize ? -INF : INF;
    result.bestMove = moves.front();
    for (const Move& move : moves)
    {
        board.makeMove(move);
        const int score = minimaxNode(board, depth - 1, 1, !maximize);
        board.undoMove();
        if ((maximize && score > best) || (!maximize && score < best))
        {
            best = score;
            result.bestMove = move;
        }
    }
    result.whiteScore = best;
    return result;
}
