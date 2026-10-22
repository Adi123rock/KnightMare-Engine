// tests/minimax_reference.h
// Phase 7 — Test-only plain minimax reference.
// Independent of production alpha-beta and move ordering.

#pragma once

#include "../engine/Board.h"
#include "../engine/move.h"

// Converts a side-to-move score from evaluate() into a white-relative score.
// This is the correct wrapper for the minimax reference.
int whiteViewEval(const Board& board);

// The result of the minimax root search.
struct MinimaxResult
{
    Move bestMove{};
    int  whiteScore = 0;   // Score from White's perspective (+ = White advantage)
};

// Plain minimax without alpha-beta.
// Returns the white-relative score (+ = good for White).
// Uses make/undo internally; leaves the board unchanged.
int minimaxSearch(Board& board, int depth, bool maximizing);

// Root minimax: finds the best move and returns white-relative score.
MinimaxResult minimaxRoot(Board& board, int depth);
