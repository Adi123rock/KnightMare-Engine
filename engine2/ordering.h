// engine2/ordering.h
// Phase 7 — Move ordering declarations.

#pragma once

#include <vector>
#include "../engine/move.h"
#include "../engine/Board.h"
#include "search_types.h"

// Score assigned to each move for ordering purposes (higher = searched first).
// Returns the ordering score for a single move.
int scoreMoveOrder(
    const Move&        move,
    const Board&       board,
    const SearchState& state,
    int                ply,
    const Move&        pvMove,
    const SearchLimits& limits
);

// Sort moves in-place from best to worst (descending order score).
void orderMoves(
    std::vector<Move>& moves,
    const Board&       board,
    const SearchState& state,
    int                ply,
    const Move&        pvMove,
    const SearchLimits& limits
);

// Update killer move table after a quiet move causes a beta cut-off.
void updateKillers(SearchState& state, int ply, const Move& move);

// Update history heuristic after a quiet move causes a beta cut-off.
void updateHistory(SearchState& state, const Move& move, SideToMove side, int depth);

// MVV-LVA table: indexed by [victim][attacker] (PieceType enum int values).
// Higher values = more valuable victim captured by cheaper attacker.
int mvvLvaScore(const Move& move, const Board& board);
