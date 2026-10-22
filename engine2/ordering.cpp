// engine2/ordering.cpp
// Phase 7 — Move ordering implementation.

#include "ordering.h"
#include "../engine/evaluation.h"   // pieceValue(), pieceType()
#include <algorithm>

// ---------------------------------------------------------------------------
// MVV-LVA (Most Valuable Victim — Least Valuable Attacker)
// ---------------------------------------------------------------------------
// Victims and attackers are indexed by PieceType enum.
// p=0, n=1, b=2, r=3, q=4, k=5

static const int MVV_LVA[6][6] =
{
    //           att:p  att:n  att:b  att:r  att:q  att:k
    /* vic:p */  { 105,  104,  103,  102,  101,  100 },
    /* vic:n */  { 205,  204,  203,  202,  201,  200 },
    /* vic:b */  { 305,  304,  303,  302,  301,  300 },
    /* vic:r */  { 405,  404,  403,  402,  401,  400 },
    /* vic:q */  { 505,  504,  503,  502,  501,  500 },
    /* vic:k */  { 605,  604,  603,  602,  601,  600 },
};

int mvvLvaScore(const Move& move, const Board& board)
{
    if (!isCapture(move) && !isEnPassant(move))
        return 0;

    char attacker = board.getPiece(move.from.r, move.from.c);
    char victim;

    if (isEnPassant(move))
    {
        // Captured pawn sits beside the moving pawn
        victim = board.getPiece(move.from.r, move.to.c);
    }
    else if (move.captured != '.' && isValid(move.captured))
    {
        victim = move.captured;
    }
    else
    {
        victim = board.getPiece(move.to.r, move.to.c);
    }

    if (!isValid(attacker) || !isValid(victim)) return 0;

    int v = static_cast<int>(pieceType(victim));
    int a = static_cast<int>(pieceType(attacker));

    return MVV_LVA[v][a];
}

// ---------------------------------------------------------------------------
// Move ordering scores
// ---------------------------------------------------------------------------
// Priority (higher = searched first):
//   PV move          : 2,000,000
//   Winning captures: 1,200,000 + MVV-LVA
//   Promotions:       1,150,000 (capture-promotion receives a further bonus)
//   Killer 0         : 800,000
//   Killer 1         : 700,000
//   History          : 0..history_max
//   Losing captures  : 100,000 + MVV-LVA

static bool movesEqual(const Move& a, const Move& b)
{
    return a.from.r    == b.from.r &&
           a.from.c    == b.from.c &&
           a.to.r      == b.to.r   &&
           a.to.c      == b.to.c   &&
           a.promotion == b.promotion;
}

int scoreMoveOrder(
    const Move&        move,
    const Board&       board,
    const SearchState& state,
    int                ply,
    const Move&        pvMove,
    const SearchLimits& limits)
{
    // PV move gets absolute top priority
    if (limits.usePvOrdering && inBounds(pvMove.from.r, pvMove.from.c) &&
        movesEqual(move, pvMove))
        return 2'000'000;

    // Promote valuable captures and promotions; defer likely losing captures
    // behind quiet killers/history while retaining MVV-LVA inside each group.
    if (limits.useCaptureOrdering && (isCapture(move) || isEnPassant(move) || isPromotion(move))) {
        const bool capture = isCapture(move) || isEnPassant(move);
        const int attacker = pieceValue(board.getPiece(move.from.r, move.from.c));
        char victim = isEnPassant(move) ? 'p' : board.getPiece(move.to.r, move.to.c);
        if (move.captured != '.' && isValid(move.captured)) victim = move.captured;
        const bool favorable = capture && pieceValue(victim) >= attacker;
        if (isPromotion(move))
            return 1'150'000 + (capture ? 250'000 + mvvLvaScore(move, board) : 0);
        if (capture)
            return (favorable ? 1'200'000 : 100'000) + mvvLvaScore(move, board);
    }

    // Killer moves (quiet moves that caused beta cut-offs at the same ply)
    int clampedPly = (ply < MAX_PLY + MAX_QPLY) ? ply : MAX_PLY + MAX_QPLY - 1;
    if (limits.useKillerOrdering &&
        movesEqual(move, state.killers[clampedPly][0]))
        return 800'000;
    if (limits.useKillerOrdering &&
        movesEqual(move, state.killers[clampedPly][1]))
        return 700'000;

    // History heuristic
    int ci = colourIndex(board.getSideToMove());
    int fi = sqIndex(move.from);
    int ti = sqIndex(move.to);
    return limits.useHistoryOrdering ? state.history[ci][fi][ti] : 0;
}

void orderMoves(
    std::vector<Move>& moves,
    const Board&       board,
    const SearchState& state,
    int                ply,
    const Move&        pvMove,
    const SearchLimits& limits)
{
    const bool enabled = limits.useCaptureOrdering || limits.usePvOrdering ||
                         limits.useKillerOrdering || limits.useHistoryOrdering;
    if (!enabled) return;

    std::stable_sort(
        moves.begin(),
        moves.end(),
        [&](const Move& a, const Move& b)
        {
            return scoreMoveOrder(a, board, state, ply, pvMove, limits) >
                   scoreMoveOrder(b, board, state, ply, pvMove, limits);
        }
    );
}

// ---------------------------------------------------------------------------
// Killer move update
// ---------------------------------------------------------------------------

void updateKillers(SearchState& state, int ply, const Move& move)
{
    // Do not store captures as killers
    if (isCapture(move) || isEnPassant(move)) return;

    int clampedPly = (ply < MAX_PLY + MAX_QPLY) ? ply : MAX_PLY + MAX_QPLY - 1;

    // Avoid duplicates
    if (movesEqual(move, state.killers[clampedPly][0])) return;

    // Shift: slot 0 -> slot 1, new move -> slot 0
    state.killers[clampedPly][1] = state.killers[clampedPly][0];
    state.killers[clampedPly][0] = move;
}

// ---------------------------------------------------------------------------
// History heuristic update
// ---------------------------------------------------------------------------

void updateHistory(SearchState& state, const Move& move, SideToMove side, int depth)
{
    if (isCapture(move) || isEnPassant(move)) return;

    int ci = colourIndex(side);
    int fi = sqIndex(move.from);
    int ti = sqIndex(move.to);

    // Bonus proportional to depth squared (common approach)
    state.history[ci][fi][ti] += depth * depth;

    // Prevent integer overflow by clamping
    if (state.history[ci][fi][ti] > 400'000)
        state.history[ci][fi][ti] = 400'000;
}
