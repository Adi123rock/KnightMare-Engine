// engine2/search.h
// Phase 7 — Search declarations.

#pragma once

#include <vector>
#include <string>

#include "../engine/Board.h"
#include "../engine/move.h"
#include "search_types.h"

// ---------------------------------------------------------------------------
// Draw detection during search
// ---------------------------------------------------------------------------

// Returns true when the current position is a draw by repetition within the
// search tree.  One earlier occurrence of the position key is sufficient.
bool repetitionSeenBefore(const Board& board, int searchPly);

// Returns true when the current position is a draw by the fifty-move rule,
// insufficient material, or stalemate.
// Note: stalemate requires move generation; call only after confirming no moves.
bool isDrawInSearch(const Board& board);

// Returns the terminal/draw score at the current node.
//   - checkmate  : matedIn(ply)
//   - stalemate/draw : 0
// Requires that the caller has already called generateLegalMoves and knows
// whether the side to move is in check.
int terminalScore(bool inCheck, int ply);

// ---------------------------------------------------------------------------
// Time and stop management
// ---------------------------------------------------------------------------

// Check stop flag or time budget and set state.stopped if necessary.
void checkStop(SearchState& state, const SearchLimits& limits, long long startMs);

// Compute a suitable time budget for the current move.
// Returns a value in milliseconds. Returns 0 if time management is disabled.
int computeTimeBudget(const SearchLimits& limits);

// Returns true when the engine can afford to begin the next depth iteration.
bool canStartNextDepth(int elapsed, int budget, int depth);

// Nodes per second (safe for zero elapsed).
long long nodesPerSecond(long long nodes, int elapsedMs);

// ---------------------------------------------------------------------------
// Principal variation
// ---------------------------------------------------------------------------

// Update the PV table at `ply` using the move just searched.
void updatePV(SearchState& state, int ply, const Move& move);

// Build a human-readable PV string from the PV table.
std::string formatPV(const SearchState& state);

// ---------------------------------------------------------------------------
// Negamax alpha-beta search
// ---------------------------------------------------------------------------

// Plain negamax without pruning — kept here as an internal recursive function.
// (The reference minimax is in tests/ only.)
// The production search uses alphaBeta below.

// Alpha-beta negamax.
// Returns the score from the side-to-move perspective.
int alphaBeta(
    Board&        board,
    SearchState&  state,
    const SearchLimits& limits,
    long long     startMs,
    int           alpha,
    int           beta,
    int           depth,
    int           ply
);

// Quiescence search: searches captures (and evasions when in check) until
// a quiet position is reached.
int quiesce(
    Board&        board,
    SearchState&  state,
    const SearchLimits& limits,
    long long     startMs,
    int           alpha,
    int           beta,
    int           qply,
    int           rootPly
);

// ---------------------------------------------------------------------------
// Root search and iterative deepening
// ---------------------------------------------------------------------------

// Generate root moves (all legal moves, optionally pre-ordered).
std::vector<Move> generateRootMoves(Board& board);

// Search from the root at a fixed depth.
// Returns the result for that depth.  Does not manage iterative deepening.
SearchResult searchRoot(
    Board&              board,
    SearchState&        state,
    const SearchLimits& limits,
    long long           startMs,
    int                 depth
);

// Full iterative deepening search.
// Calls searchRoot incrementally from depth 1 up to limits.maxDepth.
// Returns the result from the last completed depth.
SearchResult iterativeDeepening(
    Board&              board,
    const SearchLimits& limits
);

// Top-level entry point: sets up state, clears stop flag, times the search.
SearchResult search(Board& board, const SearchLimits& limits);

TranspositionTable& globalTranspositionTable();

// ---------------------------------------------------------------------------
// Output
// ---------------------------------------------------------------------------

void printSearchInfo(
    int        depth,
    int        score,
    long long  nodes,
    int        elapsedMs,
    const std::string& pvStr
);

void printBestMove(const Move& move);
