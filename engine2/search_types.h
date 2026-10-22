// engine2/search_types.h
// Phase 7 — Search types, constants, and mate-score helpers.
// This file must not modify anything in engine/.

#pragma once

#include <string>
#include <atomic>
#include <vector>
#include <cstdint>
#include <algorithm>

#include "../engine/move.h"
#include "../engine/Board.h"

// ---------------------------------------------------------------------------
// Search constants
// ---------------------------------------------------------------------------

constexpr int MATE       = 30000;   // Mate score (matches EvalParams::MATE_SCORE)
constexpr int INF        = 32001;   // Greater than MATE; used for alpha/beta init
constexpr int MAX_PLY    = 64;      // Maximum search depth / ply
constexpr int MATE_THRESHOLD = MATE - MAX_PLY; // Score above this is a mate score

// Quiescence search depth limit relative to horizon
constexpr int MAX_QPLY   = 32;

// Node-count interval between time checks
constexpr int TIME_CHECK_INTERVAL = 2048;

enum class TTBound : std::uint8_t { exact, lower, upper };

struct TTEntry {
    std::uint64_t key = 0;
    Move bestMove{};
    int score = 0;
    int depth = -1;
    int halfmoveClock = 0;
    TTBound bound = TTBound::exact;
    std::uint8_t generation = 0;
    bool quiescence = true;
    bool occupied = false;
};

struct TTStats {
    std::uint64_t probes = 0, hits = 0, cutoffs = 0, stores = 0;
};

class TranspositionTable {
public:
    explicit TranspositionTable(std::size_t megabytes = 16) { resize(megabytes); }
    void resize(std::size_t megabytes) {
        const std::size_t count = std::max<std::size_t>(1, megabytes * 1024 * 1024 / sizeof(TTEntry));
        entries_.assign(count, TTEntry{});
        generation_ = 0;
        stats_ = {};
    }
    void clear() {
        std::fill(entries_.begin(), entries_.end(), TTEntry{});
        generation_ = 0;
        stats_ = {};
    }
    void newSearch() { ++generation_; stats_ = {}; }
    TTEntry* probe(std::uint64_t key) {
        ++stats_.probes;
        if (entries_.empty()) return nullptr;
        TTEntry& entry = entries_[static_cast<std::size_t>(key % entries_.size())];
        if (!entry.occupied || entry.key != key) return nullptr;
        ++stats_.hits;
        return &entry;
    }
    void store(std::uint64_t key, int depth, int score, TTBound bound, const Move& move,
               int halfmoveClock = 0, bool quiescence = true) {
        if (entries_.empty()) return;
        TTEntry& entry = entries_[static_cast<std::size_t>(key % entries_.size())];
        if (entry.occupied && entry.key != key && entry.generation == generation_ && depth + 2 < entry.depth)
            return;
        if (entry.occupied && entry.key == key && entry.depth > depth && bound != TTBound::exact &&
            entry.generation == generation_) return;
        entry = TTEntry{key, move, score, depth, halfmoveClock, bound, generation_, quiescence, true};
        ++stats_.stores;
    }
    void recordCutoff() { ++stats_.cutoffs; }
    const TTStats& stats() const { return stats_; }
    std::size_t size() const { return entries_.size(); }
private:
    std::vector<TTEntry> entries_;
    std::uint8_t generation_ = 0;
    TTStats stats_{};
};

// ---------------------------------------------------------------------------
// SearchLimits — caller-supplied constraints for a search
// ---------------------------------------------------------------------------

struct SearchLimits
{
    int  maxDepth   = MAX_PLY;   // Maximum iterative-deepening depth
    int  timeMsec   = 0;         // Hard time budget in milliseconds (0 = unlimited)
    int  moveTime   = 0;         // Fixed time per move in milliseconds (0 = ignored)
    int  whiteTimeMs = 0;
    int  blackTimeMs = 0;
    int  whiteIncMs = 0;
    int  blackIncMs = 0;
    int  movesToGo = 0;
    std::uint64_t nodeLimit = 0;
    bool blackToMove = false;
    bool clockSpecified = false;
    bool infinite   = false;     // If true, ignore time limits (stop only on explicit stop)
    bool useQuiescence = true;   // Enable quiescence search at leaf nodes
    bool useAlphaBeta   = true;  // Disable pruning for plain negamax verification

    // Optional move-ordering heuristics. These change node order only, not
    // the search tree or score semantics, and can be disabled for benchmarks.
    bool useCaptureOrdering = true; // MVV-LVA and promotion priorities
    bool usePvOrdering      = true;
    bool useKillerOrdering  = true;
    bool useHistoryOrdering = true;
    bool useTranspositionTable = true;
    bool usePvs = false;        // Available for controlled experiments; mixed benchmark results.
    bool useAspiration = false; // Available for controlled experiments; retries regressed this set.
};

// ---------------------------------------------------------------------------
// SearchState — mutable state shared across the recursive search
// ---------------------------------------------------------------------------

struct SearchState
{
    long long  nodes        = 0;     // Nodes searched so far
    int        seldepth     = 0;     // Maximum ply reached including quiescence
    bool       stopped      = false; // Set true when search should abort
    TranspositionTable* tt = nullptr;
    std::uint64_t betaCutoffs = 0;
    std::uint64_t firstMoveCutoffs = 0;

    // Killer moves: two per ply
    Move killers[MAX_PLY + MAX_QPLY][2]{};

    // History heuristic indexed by [colour][from-sq-index][to-sq-index]
    // from/to encoded as r*8+c
    int history[2][64][64]{};

    // Triangular PV table: pvTable[ply] holds the PV from that ply onward
    // pvLength[ply] = number of moves stored at that ply
    Move pvTable[MAX_PLY + 1][MAX_PLY + 1]{};
    int  pvLength[MAX_PLY + 1]{};

    void reset()
    {
        nodes    = 0;
        seldepth = 0;
        stopped  = false;
        betaCutoffs = 0;
        firstMoveCutoffs = 0;
        for (int i = 0; i < MAX_PLY + MAX_QPLY; ++i)
        {
            killers[i][0] = Move{};
            killers[i][1] = Move{};
        }
        for (int c = 0; c < 2; ++c)
            for (int f = 0; f < 64; ++f)
                for (int t = 0; t < 64; ++t)
                    history[c][f][t] = 0;

        for (int i = 0; i <= MAX_PLY; ++i)
        {
            pvLength[i] = 0;
            for (int j = 0; j <= MAX_PLY; ++j)
                pvTable[i][j] = Move{};
        }
    }
};

// ---------------------------------------------------------------------------
// SearchResult — what the search returns to the caller
// ---------------------------------------------------------------------------

struct SearchResult
{
    Move bestMove{};          // Best move found
    int  score     = 0;       // Score in centipawns (side-to-move perspective)
    int  depth     = 0;       // Completed search depth
    long long nodes = 0;      // Total nodes searched
    int  elapsedMs = 0;       // Wall-clock time in milliseconds
    bool completed = false;  // True only when the reported depth finished
    bool interrupted = false;// Stop/time interruption occurred in this call
    std::vector<Move> principalVariation;
    std::uint64_t ttHits = 0, ttCutoffs = 0, betaCutoffs = 0, firstMoveCutoffs = 0;

    bool hasBestMove() const
    {
        // A move is considered valid when at least one of from/to is in bounds.
        return inBounds(bestMove.from.r, bestMove.from.c) &&
               inBounds(bestMove.to.r,   bestMove.to.c);
    }
};

// ---------------------------------------------------------------------------
// Mate-score helpers
// ---------------------------------------------------------------------------

// Returns the score awarded for mating the opponent in `plies` half-moves.
// Closer mates yield higher scores.
inline int mateIn(int ply)
{
    return MATE - ply;
}

// Returns the score for being mated in `plies` half-moves.
// (Negative — losing side's perspective.)
inline int matedIn(int ply)
{
    return -(MATE - ply);
}

// Returns true when |score| is in the mate range.
inline bool isMateScore(int score)
{
    return score > MATE_THRESHOLD || score < -MATE_THRESHOLD;
}

inline int normalizeMateScoreForTT(int score, int ply) {
    if (score > MATE_THRESHOLD) return score + ply;
    if (score < -MATE_THRESHOLD) return score - ply;
    return score;
}

inline int denormalizeMateScoreFromTT(int score, int ply) {
    if (score > MATE_THRESHOLD) return score - ply;
    if (score < -MATE_THRESHOLD) return score + ply;
    return score;
}

// Returns the number of half-moves to mate (positive) or to be mated (negative).
// For ordinary scores, returns 0.
inline int mateDistance(int score)
{
    if (score > MATE_THRESHOLD)  return MATE - score;   // side is winning
    if (score < -MATE_THRESHOLD) return -(MATE + score);
    return 0;
}

// Formats a score for human-readable output.
// Mate scores are shown as "M<n>" (winning) or "-M<n>" (losing).
// Ordinary scores are shown in centipawns.
inline std::string scoreToString(int score)
{
    if (score > MATE_THRESHOLD)
    {
        int movesToMate = (MATE - score + 1) / 2; // half-moves -> full moves
        return "M" + std::to_string(movesToMate);
    }
    if (score < -MATE_THRESHOLD)
    {
        int movesToMate = (MATE + score + 1) / 2;
        return "-M" + std::to_string(movesToMate);
    }
    return std::to_string(score);
}

// ---------------------------------------------------------------------------
// Colour index helper (used by history heuristic)
// ---------------------------------------------------------------------------
inline int colourIndex(SideToMove s)
{
    return s == SideToMove::white ? 0 : 1;
}

// ---------------------------------------------------------------------------
// Square index helper
// ---------------------------------------------------------------------------
inline int sqIndex(const Square& s)
{
    return s.r * 8 + s.c;
}
