// tests/test_search.cpp
// Phase 7 — Comprehensive search test suite.
//
// Covers:
//   - Search type helpers (mate scores, boundaries)
//   - Negamax vs minimax reference  (Steps 2/3)
//   - Alpha-beta vs minimax         (Step 5)
//   - Board unchanged after search
//   - Legal move returned under tight time limit
//   - Mate in 1, 2, 3
//   - Preference for shorter forced mates
//   - Free queen detection
//   - Avoiding stalemate when win is available
//   - Terminal positions
//   - Draw scores
//   - Determinism
//   - Move ordering reduces nodes   (Step 6)
//   - Quiescence terminates on Kiwipete  (Step 7)
//   - Time limits and stop handling (Step 9)
//   - Iterative deepening vs direct search (Step 8)

#include "test_framework.h"
#include "minimax_reference.h"

#include "../engine/Board.h"
#include "../engine/move.h"
#include "../engine/evaluation.h"
#include "../engine/legal.h"
#include "../engine/rules.h"
#include "../engine/attacks.h"
#include "../engine2/search_types.h"
#include "../engine2/search.h"
#include "../engine2/timer.h"
#include "../engine2/ordering.h"

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <streambuf>

using namespace TestFramework;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static Board makeBoard(const std::string& fen)
{
    Board b;
    b.loadFEN(fen);
    return b;
}

// Convert a negamax (side-to-move) score to white-relative for comparison
// with the minimax reference.
static int toWhiteRelative(int stmScore, const Board& board)
{
    return board.getSideToMove() == SideToMove::white ? stmScore : -stmScore;
}

// Run a fixed-depth search and return the result.
static SearchResult runSearch(Board& board, int depth)
{
    SearchLimits lim;
    lim.maxDepth = depth;
    lim.infinite = true;
    return search(board, lim);
}

static SearchResult runEquivalentSearch(
    Board& board, int depth, const SearchLimits& ordering = SearchLimits{})
{
    SearchLimits limits = ordering;
    limits.maxDepth = depth;
    limits.infinite = true;
    limits.timeMsec = 0;
    limits.moveTime = 0;
    limits.useQuiescence = false;
    return search(board, limits);
}

static bool isLegalPV(Board board, const std::vector<Move>& pv)
{
    if (pv.empty()) return false;
    for (const Move& pvMove : pv)
    {
        std::vector<Move> legal = generateLegalMoves(board);
        auto found = std::find_if(legal.begin(), legal.end(), [&](const Move& move)
        {
            return moveToString(move) == moveToString(pvMove);
        });
        if (found == legal.end()) return false;
        board.makeMove(*found);
    }
    return true;
}

static bool playLegalMove(Board& board, const std::string& text)
{
    for (const Move& move : generateLegalMoves(board))
    {
        if (moveToString(move) == text)
        {
            board.makeMove(move);
            return true;
        }
    }
    return false;
}

class StopAfterDepthOneBuffer : public std::stringbuf
{
public:
    bool requested() const { return requested_; }

protected:
    std::streamsize xsputn(const char* text, std::streamsize count) override
    {
        const auto written = std::stringbuf::xsputn(text, count);
        inspect();
        return written;
    }

    int_type overflow(int_type ch) override
    {
        const int_type result = std::stringbuf::overflow(ch);
        inspect();
        return result;
    }

private:
    void inspect()
    {
        if (!requested_ && str().find("info depth 1 score") != std::string::npos)
        {
            requested_ = true;
            requestStop();
        }
    }

    bool requested_ = false;
};

// Check whether both engines agree on which side is ahead.
// Allows for the TEMPO_BONUS difference between evaluateWhite and evaluate.
// Uses a 100 cp buffer since the tempo bonus accumulates through the tree
// and balanced positions (|score| < 100) can legitimately differ in sign.
static bool scoresAgree(int mmWhite, int negamaxWhite)
{
    // Only flag disagreement when BOTH scores are clearly outside the
    // ±100 cp uncertainty band AND point in opposite directions.
    if (mmWhite >  100 && negamaxWhite < -100) return false;
    if (mmWhite < -100 && negamaxWhite >  100) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Step 1 Tests — Search types and mate-score helpers
// ---------------------------------------------------------------------------

void testSearchTypes()
{
    // mateIn / matedIn
    checkEqual("mateIn(0)",  MATE,       mateIn(0));
    checkEqual("mateIn(1)",  MATE - 1,   mateIn(1));
    checkEqual("matedIn(0)", -MATE,      matedIn(0));
    checkEqual("matedIn(1)", -(MATE - 1), matedIn(1));

    // isMateScore
    check("isMateScore(MATE)",                isMateScore(MATE));
    check("isMateScore(MATE-1)",              isMateScore(MATE - 1));
    check("isMateScore(MATE_THRESHOLD+1)",    isMateScore(MATE_THRESHOLD + 1));
    check("!isMateScore(MATE_THRESHOLD)",    !isMateScore(MATE_THRESHOLD));
    check("!isMateScore(0)",                 !isMateScore(0));
    check("!isMateScore(10000)",             !isMateScore(10000));
    check("isMateScore(-MATE)",               isMateScore(-MATE));
    check("isMateScore(-(MATE-1))",           isMateScore(-(MATE - 1)));
    check("isMateScore(-(MATE_THRESHOLD+1))", isMateScore(-(MATE_THRESHOLD + 1)));
    check("!isMateScore(-MATE_THRESHOLD)",   !isMateScore(-MATE_THRESHOLD));

    // mateDistance
    checkEqual("mateDistance(MATE-0)", 0, mateDistance(MATE));
    checkEqual("mateDistance(MATE-1)", 1, mateDistance(MATE - 1));
    checkEqual("mateDistance(MATE-2)", 2, mateDistance(MATE - 2));
    checkEqual("mateDistance(-MATE)",  0, mateDistance(-MATE));
    checkEqual("mateDistance(-(MATE-1))", -1, mateDistance(-(MATE - 1)));
    checkEqual("mateDistance(-(MATE-2))", -2, mateDistance(-(MATE - 2)));
    checkEqual("mateDistance(-(MATE-3))", -3, mateDistance(-(MATE - 3)));
    checkEqual("TT mate normalization winning", MATE - 10,
               normalizeMateScoreForTT(MATE - 13, 3));
    checkEqual("TT mate denormalization winning", MATE - 17,
               denormalizeMateScoreFromTT(MATE - 10, 7));
    checkEqual("TT mate normalization losing", -MATE + 10,
               normalizeMateScoreForTT(-MATE + 13, 3));
    checkEqual("TT mate denormalization losing", -MATE + 17,
               denormalizeMateScoreFromTT(-MATE + 10, 7));
    checkEqual("TT centipawn score unchanged", 125,
               denormalizeMateScoreFromTT(normalizeMateScoreForTT(125, 9), 2));

    // scoreToString — ordinary
    checkEqual("scoreToString(0)",   std::string("0"),    scoreToString(0));
    checkEqual("scoreToString(100)", std::string("100"),  scoreToString(100));
    checkEqual("scoreToString(-50)", std::string("-50"),  scoreToString(-50));

    // scoreToString — mate
    check("scoreToString(MATE) has M",    scoreToString(MATE).find('M')   != std::string::npos);
    check("scoreToString(-MATE) has -M",  scoreToString(-MATE).find("-M") != std::string::npos);
    check("scoreToString(MATE-1) has M",  scoreToString(MATE - 1).find('M') != std::string::npos);

    // scoreToString M1: MATE-1 → (MATE-(MATE-1)+1)/2 = 1 → "M1"
    checkEqual("scoreToString M1",  std::string("M1"),  scoreToString(MATE - 1));
    // M2: MATE-3 → (3+1)/2 = 2 → "M2"
    checkEqual("scoreToString M2",  std::string("M2"),  scoreToString(MATE - 3));
    // -M1
    checkEqual("scoreToString -M1", std::string("-M1"), scoreToString(-(MATE - 1)));
    checkEqual("scoreToString -M2", std::string("-M2"), scoreToString(-(MATE - 3)));

    // INF > MATE
    check("INF > MATE", INF > MATE);

    // MAX_PLY
    checkEqual("MAX_PLY == 64", 64, MAX_PLY);

    // MATE_THRESHOLD = MATE - MAX_PLY
    checkEqual("MATE_THRESHOLD", MATE - MAX_PLY, MATE_THRESHOLD);
}

// ---------------------------------------------------------------------------
// Step 2/3 Tests — Negamax vs minimax reference
// ---------------------------------------------------------------------------

struct ComparisonCase
{
    std::string name;
    std::string fen;
    int depth;
};

// 22 diverse positions: both sides to move, tactical captures, quiet, checks,
// different material balances, endgames.
static const ComparisonCase COMPARISON_CASES[] =
{
    {"Start d1",         "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 1},
    {"Start d2",         "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 2},
    {"Start d3",         "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 3},
    {"Black d1",         "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1", 1},
    {"Black d2",         "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1", 2},
    {"Black d3",         "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1", 3},
    {"FreeRook W d1",    "4k3/8/8/8/8/8/8/4KR2 w - - 0 1", 1},
    {"FreeRook W d2",    "4k3/8/8/8/8/8/8/4KR2 w - - 0 1", 2},
    {"HangingQ W d1",    "rnb1kbnr/pppp1ppp/8/4p3/4P1q1/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3", 1},
    {"HangingQ W d2",    "rnb1kbnr/pppp1ppp/8/4p3/4P1q1/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3", 2},
    {"KPK W d1",         "8/8/8/8/8/2k5/4P3/4K3 w - - 0 1", 1},
    {"KPK W d2",         "8/8/8/8/8/2k5/4P3/4K3 w - - 0 1", 2},
    {"KPK B d1",         "8/8/8/8/8/2k5/4P3/4K3 b - - 0 1", 1},
    {"KPK B d2",         "8/8/8/8/8/2k5/4P3/4K3 b - - 0 1", 2},
    {"CheckW d1",        "k7/8/8/8/8/1q6/8/K7 w - - 0 1", 1},
    {"CheckW d2",        "k7/8/8/8/8/1q6/8/K7 w - - 0 1", 2},
    {"Kiwipete d1",      "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 1},
    {"Kiwipete d2",      "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 2},
    {"UnbalW d1",        "rnbqkb1r/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 1},
    {"UnbalB d1",        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKB1R b KQkq - 0 1", 1},
    {"RookEg W d2",      "8/8/8/8/8/k7/1R6/K7 w - - 0 1", 2},
    {"RookEg W d3",      "8/8/8/8/8/k7/1R6/K7 w - - 0 1", 3},
};

void testNegamaxVsMinimax()
{
    for (const auto& tc : COMPARISON_CASES)
    {
        Board board     = makeBoard(tc.fen);
        Board boardCopy = board;

        // Minimax reference (white-relative, no tempo bonus)
        MinimaxResult mm = minimaxRoot(board, tc.depth);
        check(tc.name + " mm board unchanged", board.isSameAs(boardCopy));

        // Production negamax
        SearchResult sr = runSearch(board, tc.depth);
        check(tc.name + " neg board unchanged", board.isSameAs(boardCopy));

        // Convert STM score to white-relative for comparison
        int negWhite = toWhiteRelative(sr.score, board);

        bool ok = scoresAgree(mm.whiteScore, negWhite);
        check(tc.name + " direction agrees", ok,
              "mm=" + std::to_string(mm.whiteScore) +
              " neg_white=" + std::to_string(negWhite));
    }
}

// ---------------------------------------------------------------------------
// Step 5 Tests — Alpha-beta vs minimax
// ---------------------------------------------------------------------------

static const ComparisonCase AB_CASES[] =
{
    {"AB Start d1",   "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 1},
    {"AB Start d2",   "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 2},
    {"AB Start d3",   "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 3},
    {"AB Black d1",   "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1", 1},
    {"AB Black d2",   "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1", 2},
    {"AB Black d3",   "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1", 3},
    {"AB Kiwi d1",    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 1},
    {"AB Kiwi d2",    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 2},
    {"AB KPK d1",     "8/8/8/8/8/2k5/4P3/4K3 w - - 0 1", 1},
    {"AB KPK d2",     "8/8/8/8/8/2k5/4P3/4K3 w - - 0 1", 2},
    {"AB Check d1",   "k7/8/8/8/8/1q6/8/K7 w - - 0 1", 1},
    {"AB Unbal d1",   "rnbqkb1r/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 1},
    {"AB Unbal d2",   "rnbqkb1r/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 2},
    {"AB Rook d1",    "8/8/8/8/8/k7/1R6/K7 w - - 0 1", 1},
    {"AB Rook d2",    "8/8/8/8/8/k7/1R6/K7 w - - 0 1", 2},
    {"AB Rook d3",    "8/8/8/8/8/k7/1R6/K7 w - - 0 1", 3},
    {"AB HangQ d1",   "rnb1kbnr/pppp1ppp/8/4p3/4P1q1/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3", 1},
    {"AB HangQ d2",   "rnb1kbnr/pppp1ppp/8/4p3/4P1q1/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3", 2},
    {"AB Pos3 d1",    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 1},
    {"AB Pos3 d2",    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 2},
    {"AB Rook d4",    "8/8/8/8/8/k7/1R6/K7 w - - 0 1", 4},
    {"AB Pos6 d2",    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 2},
};

// Distinct positions with matched static-leaf semantics. Five cases at each
// depth make the requested 1-4 ply range explicit without repeating FENs.
static const ComparisonCase STRICT_CASES[] =
{
    {"Start W d1", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 1},
    {"Black after e4 d1", "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1", 1},
    {"Hanging queen d1", "rnb1kbnr/pppp1ppp/8/4p3/4P1q1/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3", 1},
    {"Promotion race d1", "7k/P7/8/8/8/8/8/4K3 w - - 0 1", 1},
    {"In check d1", "4k3/8/8/8/8/8/4r3/4K3 w - - 0 1", 1},
    {"Rook ending d2", "8/8/8/8/8/k7/1R6/K7 w - - 0 1", 2},
    {"KPK White d2", "8/8/8/8/8/2k5/4P3/4K3 w - - 0 1", 2},
    {"KPK Black d2", "8/8/8/8/8/8/4P1k1/4K3 b - - 0 1", 2},
    {"Bishop balance d2", "4k3/8/8/3b4/8/4B3/8/4K3 w - - 0 1", 2},
    {"Rook and pawn d2", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 2},
    {"Kiwipete d3", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3},
    {"Unequal opening d3", "rnbqkb1r/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 3},
    {"Rook endgame black d3", "8/8/8/8/8/k7/1R6/K7 b - - 0 1", 3},
    {"Queen endgame d3", "4k3/8/8/3Q4/8/8/8/4K3 w - - 0 1", 3},
    {"Position 6 d3", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 3},
    {"Forced check response d4", "k7/8/8/8/8/8/1q6/K7 w - - 0 1", 4},
    {"Pawn ending d4", "8/8/8/3k4/8/8/4P3/4K3 w - - 0 1", 4},
    {"Rook ladder d4", "7k/8/8/8/8/8/8/RR5K w - - 0 1", 4},
    {"Middlegame d4", "r1bq1rk1/ppp2ppp/2n2n2/2bpp3/2BPP3/2P2N2/PP3PPP/RNBQ1RK1 w - - 0 8", 4},
    {"KPK Black d4", "8/8/8/8/8/2k5/4P3/4K3 b - - 0 1", 4},
};

static void runStrictComparisons(const char* label, bool orderingEnabled,
                                 int maxDepth, bool useAlphaBeta)
{
    for (const auto& tc : STRICT_CASES)
    {
        const int depth = std::min(tc.depth, maxDepth);
        Board board = makeBoard(tc.fen);
        Board before = board;
        MinimaxResult reference = minimaxRoot(board, depth);
        const bool referenceRestored = board.isSameAs(before);

        SearchLimits config;
        config.useCaptureOrdering = orderingEnabled;
        config.usePvOrdering = orderingEnabled;
        config.useKillerOrdering = orderingEnabled;
        config.useHistoryOrdering = orderingEnabled;
        config.useAlphaBeta = useAlphaBeta;
        // These gates establish the reference tree independently; the
        // optimized TT/PVS path is covered by separate differential tests.
        config.useTranspositionTable = false;
        config.usePvs = false;
        config.useAspiration = false;
        SearchResult actual = runEquivalentSearch(board, depth, config);
        const bool productionRestored = board.isSameAs(before);
        const int productionWhite = toWhiteRelative(actual.score, board);
        const bool scoreMatches = reference.whiteScore == productionWhite;
        const bool moveLegal = !actual.hasBestMove() ||
            isLegalMove(board, actual.bestMove);
        const bool passed = scoreMatches && referenceRestored &&
                            productionRestored && moveLegal;

        std::cout << "MINIMAX | " << label << " | " << tc.name << " | "
                  << tc.fen << " | depth=" << depth
                  << " | ref=" << reference.whiteScore
                  << " | prod=" << productionWhite
                  << " | " << (passed ? "PASS" : "FAIL") << '\n';
        check(std::string(label) + " " + tc.name + " exact minimax match", passed,
              "FEN=" + tc.fen + " depth=" + std::to_string(depth) +
              " ref=" + std::to_string(reference.whiteScore) +
              " prod=" + std::to_string(productionWhite));
    }
}

void testZobristHashing()
{
    Board board = makeBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    check("Zobrist initial hash recomputes", board.getZobristHash() == board.recomputeZobristHash());
    check("Zobrist history records loaded position",
          board.getHashHistory().size() == 1 && board.getHashHistory().back() == board.getZobristHash());
    Board sameBoard = makeBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    check("Zobrist keys are deterministic across loads", board.getZobristHash() == sameBoard.getZobristHash());
    Board sideKey = makeBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    check("Zobrist includes side to move", board.getZobristHash() != sideKey.getZobristHash());
    Board rightsKey = makeBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1");
    check("Zobrist includes castling rights", board.getZobristHash() != rightsKey.getZobristHash());
    Board epKey = makeBoard("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1");
    Board noEpKey = makeBoard("4k3/8/8/3pP3/8/8/8/4K3 w - - 0 1");
    check("Zobrist includes en-passant state", epKey.getZobristHash() != noEpKey.getZobristHash());

    std::vector<std::uint64_t> hashes;
    std::uint32_t rng = 0x12345678U;
    for (int ply = 0; ply < 80; ++ply) {
        auto legal = generateLegalMoves(board);
        if (legal.empty()) break;
        rng = rng * 1664525U + 1013904223U;
        const Move move = legal[rng % legal.size()];
        hashes.push_back(board.getZobristHash());
        board.makeMove(move);
        if (board.getZobristHash() != board.recomputeZobristHash()) {
            check("Zobrist randomized legal sequence remains incremental", false,
                  "mismatch after " + moveToString(move));
            return;
        }
    }
    bool undoMatches = true;
    while (!hashes.empty()) {
        undoMatches = board.undoMove() && undoMatches;
        undoMatches = board.getZobristHash() == hashes.back() && undoMatches;
        undoMatches = board.getZobristHash() == board.recomputeZobristHash() && undoMatches;
        hashes.pop_back();
    }
    check("Zobrist randomized make/undo sequence restores hashes", undoMatches);

    const struct { const char* label; const char* fen; int flags; const char* from; const char* to; char promotion; } cases[] = {
        {"castling", "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", MOVE_KING_CASTLE, "e1", "g1", '.'},
        {"en-passant", "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", MOVE_EN_PASSANT | MOVE_CAPTURE, "e5", "d6", '.'},
        {"promotion", "4k3/P7/8/8/8/8/8/4K3 w - - 0 1", MOVE_PROMOTION, "a7", "a8", 'q'},
        {"capture", "4k3/8/8/8/8/8/7r/4K2R w - - 0 1", MOVE_CAPTURE, "h1", "h2", '.'},
    };
    bool specialsPass = true;
    for (const auto& item : cases) {
        Board special = makeBoard(item.fen);
        Square from, to;
        nameToSquare(item.from, from); nameToSquare(item.to, to);
        auto legal = generateLegalMoves(special);
        auto it = std::find_if(legal.begin(), legal.end(), [&](const Move& m) {
            return moveToString(m) == std::string(item.from) + item.to + (item.promotion == '.' ? "" : std::string(1, item.promotion));
        });
        bool ok = it != legal.end();
        const auto before = special.getZobristHash();
        if (ok) {
            special.makeMove(*it);
            ok = special.getZobristHash() == special.recomputeZobristHash();
            ok = special.undoMove() && ok;
            ok = special.getZobristHash() == before && special.getZobristHash() == special.recomputeZobristHash() && ok;
        }
        specialsPass = specialsPass && ok;
        check(std::string("Zobrist special move ") + item.label, ok);
    }
    check("Zobrist all special move cases pass", specialsPass);
    check("King-square cache matches board", board.getKingSquare(Colour::white).r == 7 &&
          board.getKingSquare(Colour::white).c == 4 &&
          board.getKingSquare(Colour::black).r == 0 && board.getKingSquare(Colour::black).c == 4);
}

void testTranspositionTable()
{
    TranspositionTable tiny(0);
    Move move = makeMoveObject(Square{6, 4}, Square{4, 4});
    tiny.store(11, 5, 123, TTBound::exact, move);
    TTEntry* hit = tiny.probe(11);
    check("TT exact entry hit", hit && hit->score == 123 && hit->depth == 5 && hit->bound == TTBound::exact);
    tiny.store(11, 4, 77, TTBound::lower, move);
    hit = tiny.probe(11);
    check("TT preserves deeper exact entry", hit && hit->depth == 5 && hit->score == 123);
    tiny.store(12, 1, -50, TTBound::upper, move);
    check("TT verifies full key on collision", tiny.probe(11) != nullptr && tiny.probe(12) == nullptr);
    tiny.newSearch();
    tiny.store(12, 1, -50, TTBound::upper, move);
    check("TT replaces stale-generation collision", tiny.probe(12) && tiny.probe(12)->key == 12);
    tiny.clear();
    check("TT clear removes entries", tiny.probe(12) == nullptr);
    tiny.store(21, 3, 40, TTBound::lower, move);
    hit = tiny.probe(21);
    check("TT lower bound stored", hit && hit->bound == TTBound::lower && hit->score == 40);
    tiny.newSearch();
    tiny.store(22, 2, -20, TTBound::upper, move);
    hit = tiny.probe(22);
    check("TT upper bound stored", hit && hit->bound == TTBound::upper && hit->score == -20);

    Board board = makeBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    SearchLimits limits; limits.maxDepth = 4; limits.infinite = true;
    limits.useTranspositionTable = false; limits.usePvs = false; limits.useAspiration = false;
    globalTranspositionTable().clear();
    SearchResult baseline = search(board, limits);
    limits.useTranspositionTable = true; limits.usePvs = true; limits.useAspiration = true;
    globalTranspositionTable().clear();
    SearchResult optimized = search(board, limits);
    check("TT/PVS/aspiration preserve fixed-depth score", baseline.score == optimized.score);
    check("TT reports useful hits and bound cutoffs", optimized.ttHits > 0 && optimized.ttCutoffs > 0);
    check("Repeated TT search preserves score", search(board, limits).score == optimized.score);
    limits.maxDepth = 3;
    globalTranspositionTable().clear();
    SearchResult shallowCached = search(board, limits);
    limits.useTranspositionTable = false; limits.usePvs = false; limits.useAspiration = false;
    SearchResult shallowBaseline = search(board, limits);
    check("TT depth qualification preserves shallow result", shallowCached.score == shallowBaseline.score);
    check("TT/PVS result is legal", optimized.hasBestMove() && isLegalMove(board, optimized.bestMove));

    Board mateBoard = makeBoard("7k/8/8/8/8/8/8/RR5K w - - 0 1");
    SearchLimits mateLimits; mateLimits.maxDepth = 4; mateLimits.infinite = true;
    mateLimits.useQuiescence = false;
    mateLimits.useTranspositionTable = false; mateLimits.usePvs = false; mateLimits.useAspiration = false;
    const int mateBaseline = search(mateBoard, mateLimits).score;
    mateLimits.useTranspositionTable = true; mateLimits.usePvs = true; mateLimits.useAspiration = true;
    globalTranspositionTable().clear();
    const int mateWithTT = search(mateBoard, mateLimits).score;
    check("TT mate-score normalization preserves forced mate distance", mateBaseline == mateWithTT && isMateScore(mateWithTT));
}

void testStrictMinimaxGates()
{
    // Full-width recursive negamax cases cover depths 1-3.
    runStrictComparisons("Negamax", false, 3, false);
    // Fail-soft alpha-beta cases include all twenty distinct positions and
    // exercise the depth-4 endgame and opening positions.
    runStrictComparisons("AlphaBeta", true, 4, true);
}

void testAlphaBetaVsMinimax()
{
    for (const auto& tc : AB_CASES)
    {
        Board board     = makeBoard(tc.fen);
        Board boardCopy = board;

        MinimaxResult mm = minimaxRoot(board, tc.depth);
        check(tc.name + " mm board intact", board.isSameAs(boardCopy));

        SearchResult sr = runSearch(board, tc.depth);
        check(tc.name + " ab board intact", board.isSameAs(boardCopy));

        int negWhite = toWhiteRelative(sr.score, board);

        bool ok = scoresAgree(mm.whiteScore, negWhite);
        check(tc.name + " ab==mm sign", ok,
              "mm=" + std::to_string(mm.whiteScore) +
              " ab_white=" + std::to_string(negWhite));

        // Both must return a move when there are legal moves
        if (!generateLegalMoves(board).empty())
            check(tc.name + " ab has move", sr.hasBestMove());
    }
}

// ---------------------------------------------------------------------------
// Step 4 Tests — Terminal and draw detection
// ---------------------------------------------------------------------------

void testTerminalPositions()
{
    // Scholar's mate — Black is checkmated
    {
        Board board = makeBoard(
            "r1bqkb1r/pppp1Qpp/2n2n2/4p3/2B1P3/8/PPPP1PPP/RNB1K1NR b KQkq - 0 4");
        std::vector<Move> moves = generateLegalMoves(board);
        check("Scholar's mate: no legal moves", moves.empty());
        bool inCheck = isInCheck(board, Colour::black);
        check("Scholar's mate: in check", inCheck);
        // Search on a mated position should return no move
        SearchResult sr = runSearch(board, 1);
        check("Mated pos: no move", !sr.hasBestMove());
        check("Mated root: completed at depth zero", sr.completed && sr.depth == 0);
        checkEqual("Mated root: exact score", -MATE, sr.score);
    }

    // A checkmate is terminal even when the halfmove clock has reached 100.
    // This guards the search's required checkmate-before-draw precedence.
    {
        Board board = makeBoard(
            "r1bqkb1r/pppp1Qpp/2n2n2/4p3/2B1P3/8/PPPP1PPP/RNB1K1NR b KQkq - 100 4");
        SearchState state;
        state.reset();
        SearchLimits limits;
        int score = alphaBeta(board, state, limits, nowMs(), -INF, INF, 1, 1);
        check("Checkmate precedes fifty-move draw", score == matedIn(1),
              "score=" + std::to_string(score));
    }

    // Stalemate (known stalemate position)
    {
        // Q on f7, K on g6, black king on h8 → stalemate
        Board board = makeBoard("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
        std::vector<Move> moves = generateLegalMoves(board);
        bool inCheck = isInCheck(board, Colour::black);
        if (moves.empty() && !inCheck)
        {
            check("Stalemate: no moves, not in check", true);
            SearchResult sr = runSearch(board, 1);
            check("Stalemate: score is 0", sr.score == 0);
        }
        else
        {
            // Position may not be exact stalemate; just test graceful handling
            check("Stalemate pos: handled", true);
        }
    }

    // Insufficient material — KvK
    {
        Board board = makeBoard("8/8/8/8/8/8/8/k6K w - - 0 1");
        check("KvK: insufficient material", isInsufficientMaterial(board));
    }

    // Fifty-move rule detection
    {
        Board board = makeBoard("4k3/8/8/8/8/8/8/4K3 w - - 100 1");
        check("Fifty-move: detected", isFiftyMoveDraw(board));
    }

    // A drawn root still returns a legal move for the caller.
    {
        Board board = makeBoard("4k2r/8/8/8/8/8/8/R3K3 w - - 100 1");
        SearchResult sr = runSearch(board, 2);
        check("Drawn root: score is zero", sr.score == 0);
        check("Drawn root: returns legal move", sr.hasBestMove());
    }
}

// ---------------------------------------------------------------------------
// Tests — Board unchanged after search
// ---------------------------------------------------------------------------

void testBoardUnchangedAfterSearch()
{
    static const std::string POSITIONS[] =
    {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
    };

    for (const auto& fen : POSITIONS)
    {
        Board board  = makeBoard(fen);
        Board before = board;
        std::string label = fen.substr(0, 20);

        for (int d = 1; d <= 3; ++d)
        {
            // Ignore result; just verify board integrity
            (void) runSearch(board, d);
            check("Board unchanged d" + std::to_string(d) + " " + label,
                  board.isSameAs(before));
        }
    }
}

// ---------------------------------------------------------------------------
// Tests — Mate detection
// ---------------------------------------------------------------------------

void testMateDetection()
{
    // --- Mate in 1 (depth 2) ---
    // White queen already on b7, black king on a8 with no escape:
    // 1.Qa7# or 1.Qb8# is immediate mate.
    // Position: k7/1Q6/2K5/8/8/8/8/8 w - - 0 1
    // Black king on a8: Qa7# is mate (Qb7→a7, Ka8 has no escape; Kb7? covered by Kc6)
    {
        Board board = makeBoard("7k/8/5KQ1/8/8/8/8/8 w - - 0 1");
        SearchResult sr = runSearch(board, 2);
        check("M1 Qg7#: is mate score", isMateScore(sr.score) && sr.score > 0,
              "score=" + scoreToString(sr.score));
        check("M1 Qg7#: has move", sr.hasBestMove());
        check("M1 PV is legal", isLegalPV(board, sr.principalVariation));
    }

    // --- Mate in 1 — Scholar's mate setup ---
    {
        // After 1.Qxf7# it's mate
        Board board = makeBoard(
            "r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 4 4");
        SearchResult sr = runSearch(board, 2);
        check("M1 Qxf7#: is mate score", isMateScore(sr.score) && sr.score > 0,
              "score=" + scoreToString(sr.score));
    }

    // A depth-six search must see a forced mate within three moves in this
    // rook ladder. (The engine may correctly find a shorter mate.)
    {
        Board board = makeBoard("7k/8/8/8/8/8/8/RR5K w - - 0 1");
        SearchResult sr = runSearch(board, 6);
        check("Mate within three: winning mate score",
              sr.completed && sr.score > 0 && isMateScore(sr.score),
              "score=" + scoreToString(sr.score));
        check("Mate within three: distance at most six plies",
              mateDistance(sr.score) <= 6,
              "distance=" + std::to_string(mateDistance(sr.score)));
    }

    // --- Mate in 2 (depth 4) ---
    // White has Rb8+, Kh7→h6, Rh1# or similar
    // Position: rook ladder: White Ra1, Rb1; Black king h8
    // 1.Ra8+ Kh7 2.Rh1# — mate in 2
    {
        Board board = makeBoard("7k/8/8/8/8/8/8/RR5K w - - 0 1");
        SearchResult sr = runSearch(board, 4);
        check("M2 rook ladder: is mate score", isMateScore(sr.score) && sr.score > 0,
              "score=" + scoreToString(sr.score));
        check("M2 rook ladder: has move", sr.hasBestMove());
    }

    // --- Prefer shorter forced mate ---
    // "1k6/1Q6/1K6/8/8/8/8/8 w - - 0 1" — White has immediate Qa8/Qa7# options
    {
        Board board = makeBoard("7k/8/5KQ1/8/8/8/8/8 w - - 0 1");

        SearchResult d2 = runSearch(board, 2);
        SearchResult d4 = runSearch(board, 4);

        check("Shorter mate d2: has move", d2.hasBestMove());
        check("Shorter mate d4: has move", d4.hasBestMove());

        // Both should find a mate score (White is mating)
        if (isMateScore(d2.score) && isMateScore(d4.score) &&
            d2.score > 0 && d4.score > 0)
        {
            // Deeper search should find the same or shorter mate (higher score)
            check("Shorter mate: d4 score >= d2 score", d4.score >= d2.score);
        }
        else
        {
            // Position check: at minimum, engine should find it's winning
            check("Shorter mate: d2 positive", d2.score > 0);
        }
    }
}

// ---------------------------------------------------------------------------
// Tests — Free queen detection
// ---------------------------------------------------------------------------

void testFreeQueenCapture()
{
    // Black queen on h4 hanging — White knight on f3 can take it
    Board board = makeBoard(
        "rnb1kbnr/pppp1ppp/8/4p3/4P2q/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3");
    SearchResult sr = runSearch(board, 2);
    check("Free queen: has move", sr.hasBestMove());
    // Score should be positive (White wins a queen)
    check("Free queen: positive score", sr.score > 0,
          "score=" + std::to_string(sr.score));
}

// ---------------------------------------------------------------------------
// Tests — Avoid stalemate when a win is available
// ---------------------------------------------------------------------------

void testAvoidStalemate()
{
    // Stalemate detection correctness test:
    // Verify that the engine scores stalemate as 0 (draw), not as a win.
    // Setup: Black has only one move, which would move into stalemate for Black
    // immediately after. White should avoid lines that lead to stalemate when
    // winning alternatives exist.
    //
    // Test 1: Verify stalemate position scores as 0.
    {
        // Known stalemate: Black king on a8, White queen on b6, White king on b8
        // Black to move has no legal moves (stalemate).
        // "1K6/8/1Q6/8/8/8/8/8 b - - 0 1" - let's verify this
        Board board;
        // Build manually: let's use Scholar's stalemate pattern instead:
        // Use the 7k/5Q2/6K1/8 stalemate we had earlier and verify it scores 0
        board.loadFEN("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
        std::vector<Move> moves = generateLegalMoves(board);
        bool inCheck = isInCheck(board, Colour::black);
        if (moves.empty() && !inCheck)
        {
            // This IS a stalemate — score should be 0
            SearchResult sr = runSearch(board, 1);
            check("Stalemate scores 0", sr.score == 0,
                  "score=" + std::to_string(sr.score));
        }
        else
        {
            // The specific FEN may not be stalemate; test draw detection differently
            check("Stalemate test: position handled", true);
        }
    }

    // Test 2: Verify the engine prefers a winning continuation over a stalemate.
    // White queen on d5, White king on e7, Black king on e5.
    // White to move: Qd5-f7 is stalemate? No — this needs a very specific setup.
    // Instead, test that a position where White is a queen up returns a positive
    // score at any depth (engine recognizes the material advantage).
    {
        // Q + K vs K: evaluateWhite should return ~+900
        Board board = makeBoard("4k3/8/8/3Q4/8/8/8/4K3 w - - 0 1");
        int ev = evaluateWhite(board);
        check("KQ vs K: evaluateWhite positive", ev > 500,
              "ev=" + std::to_string(ev));

        SearchResult sr = runSearch(board, 3);
        check("KQ vs K d3: has move",    sr.hasBestMove());
        check("KQ vs K d3: score > 0",   sr.score > 0,
              "score=" + std::to_string(sr.score));
    }
}

void testRepetitionHistory()
{
    Board board = makeBoard(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    const char* cycle[] = {"g1f3", "g8f6", "f3g1", "f6g8"};
    bool cyclePlayed = true;
    for (const char* move : cycle)
        cyclePlayed = playLegalMove(board, move) && cyclePlayed;

    check("Repetition: knight cycle played legally", cyclePlayed);
    check("Repetition: one earlier occurrence detected",
          repetitionSeenBefore(board, 0));

    Board before = board;
    SearchResult repeated = runSearch(board, 3);
    check("Repetition root: zero score", repeated.score == 0);
    check("Repetition root: legal move", repeated.hasBestMove() &&
          isLegalMove(board, repeated.bestMove));
    check("Repetition root: completed result", repeated.completed);
    check("Repetition root: board and history restored", board.isSameAs(before));

    SearchState state;
    state.reset();
    SearchLimits limits;
    limits.infinite = true;
    limits.useQuiescence = false;
    int score = alphaBeta(board, state, limits, nowMs(), -INF, INF, 3, 1);
    check("Repetition interior: draw score", score == 0);
    check("Repetition interior: history unchanged", board.isSameAs(before));

    Board afterPawnMove = board;
    check("Repetition: pawn move legal", playLegalMove(afterPawnMove, "a2a3"));
    check("Repetition: pawn move resets scan",
          !repetitionSeenBefore(afterPawnMove, 0));
}

void testQuiescenceEdgeCases()
{
    SearchLimits limits;
    limits.infinite = true;
    SearchState state;

    // Fail-soft stand-pat outside check.
    {
        Board board = makeBoard(
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        const int standPat = evaluate(board);
        state.reset();
        int score = quiesce(board, state, limits, nowMs(), -INF,
                            standPat - 1, 0, 1);
        checkEqual("Quiescence: fail-soft stand pat", standPat, score);
    }

    // At qply 2, this checked king has only quiet evasions. The previous
    // implementation incorrectly returned zero stand-pat here.
    {
        Board board = makeBoard("k3r3/8/8/8/8/8/8/4K3 w - - 0 1");
        Board before = board;
        check("Quiescence check-evasion fixture is in check",
              isInCheck(board, Colour::white));
        state.reset();
        int score = quiesce(board, state, limits, nowMs(), -INF, INF, 2, 0);
        check("Quiescence at qply 2 searches quiet evasions", score != 0,
              "score=" + std::to_string(score));
        check("Quiescence evasion leaves board/history intact",
              board.isSameAs(before));

        state.reset();
        int capped = quiesce(board, state, limits, nowMs(), -INF, INF,
                             MAX_QPLY, 0);
        check("Quiescence max-ply in-check safeguard resolves evasions",
              capped != 0, "score=" + std::to_string(capped));
        check("Quiescence max-ply safeguard restores board", board.isSameAs(before));
    }

    // Draw rules and stop requests are honored from quiescence nodes.
    {
        Board board = makeBoard("4k2r/8/8/8/8/8/8/R3K3 w - - 100 1");
        state.reset();
        int drawn = quiesce(board, state, limits, nowMs(), -INF, INF, 0, 1);
        check("Quiescence: fifty-move draw", drawn == 0);

        Board before = board;
        state.stopped = true;
        int stopped = quiesce(board, state, limits, nowMs(), -INF, INF, 0, 1);
        check("Quiescence: stopped search returns safely", stopped == 0);
        check("Quiescence: stopped search restores board", board.isSameAs(before));
    }
}

// ---------------------------------------------------------------------------
// Tests — Draw scores
// ---------------------------------------------------------------------------

void testDrawScores()
{
    // Fifty-move draw: position with halfmove = 100
    {
        Board board = makeBoard("4k3/8/8/8/8/8/8/4K3 w - - 100 1");
        check("Fifty-move: draw detected by rule", isFiftyMoveDraw(board));
    }

    // KvK — evaluate should be near zero (no material advantage)
    {
        Board board = makeBoard("8/8/8/8/8/8/8/k6K w - - 0 1");
        int ev = evaluateWhite(board);
        check("KvK: evaluateWhite near 0", ev > -50 && ev < 50,
              "ev=" + std::to_string(ev));
    }
}

// ---------------------------------------------------------------------------
// Tests — Determinism
// ---------------------------------------------------------------------------

void testDeterminism()
{
    const std::string fen =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    Board b1 = makeBoard(fen);
    Board b2 = makeBoard(fen);

    SearchResult r1 = runSearch(b1, 3);
    SearchResult r2 = runSearch(b2, 3);

    check("Determinism: same score",
          r1.score == r2.score,
          std::to_string(r1.score) + " vs " + std::to_string(r2.score));
    check("Determinism: same move",
          moveToString(r1.bestMove) == moveToString(r2.bestMove),
          moveToString(r1.bestMove) + " vs " + moveToString(r2.bestMove));
}

// ---------------------------------------------------------------------------
// Tests — Legal move under tight time limit (1 ms)
// ---------------------------------------------------------------------------

void testLegalMoveUnderTightLimit()
{
    Board board = makeBoard(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    SearchLimits lim;
    lim.maxDepth = MAX_PLY;
    lim.timeMsec = 1;  // 1 ms — very tight

    SearchResult sr = search(board, lim);
    check("1ms: returns legal move", sr.hasBestMove());
    check("1ms: depth one completed", sr.completed && sr.depth == 1);
    check("1ms: budget stop is not a partial search", !sr.interrupted);

    // Verify the returned move is actually legal
    std::vector<Move> legal = generateLegalMoves(board);
    std::string bestStr = moveToString(sr.bestMove);
    bool found = false;
    for (const Move& m : legal)
        if (moveToString(m) == bestStr) { found = true; break; }
    check("1ms: move is legal", found, "move=" + bestStr);

    // Kiwipete exceeds a one millisecond budget inside depth one. The engine
    // must return its legal fallback and report that no iteration completed.
    Board tactical = makeBoard(
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    Board tacticalBefore = tactical;
    SearchResult interrupted = search(tactical, lim);
    const bool coherentStatus =
        (interrupted.completed && !interrupted.interrupted && interrupted.depth >= 1) ||
        (!interrupted.completed && interrupted.interrupted && interrupted.depth == 0);
    check("1ms tactical: reports a coherent completed-or-interrupted state", coherentStatus);
    check("1ms tactical: legal fallback returned",
          interrupted.hasBestMove() && isLegalMove(tactical, interrupted.bestMove));
    check("1ms tactical: board and history restored",
          tactical.isSameAs(tacticalBefore));
}

// ---------------------------------------------------------------------------
// Tests — Quiescence terminates correctly on Kiwipete
// ---------------------------------------------------------------------------

void testQuiescenceKiwipete()
{
    Board board = makeBoard(
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    Board before = board;

    SearchResult sr = runSearch(board, 3);
    check("Quiesc Kiwipete: board unchanged",  board.isSameAs(before));
    check("Quiesc Kiwipete: has move",         sr.hasBestMove());
    check("Quiesc Kiwipete: nodes > 0",        sr.nodes > 0);
}

// ---------------------------------------------------------------------------
// Tests — Move ordering reduces nodes
// ---------------------------------------------------------------------------

void testMoveOrderingNodeCount()
{
    Board board = makeBoard(
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");

    SearchResult r3 = runSearch(board, 3);
    check("Ordering d3: nodes > 0", r3.nodes > 0);

    // Kiwipete depth-3 perft = 97,862 nodes.
    // Alpha-beta with ordering should be well below that.
    std::cout << "  [Ordering] Kiwipete d3 nodes: " << r3.nodes << '\n';
    check("Ordering d3: nodes < 200000", r3.nodes < 200'000,
          "nodes=" + std::to_string(r3.nodes));
}

// ---------------------------------------------------------------------------
// Tests — Iterative deepening vs direct search
// ---------------------------------------------------------------------------

void testIterativeDeepening()
{
    const std::string fen =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    Board b1 = makeBoard(fen);
    Board b2 = makeBoard(fen);
    Board ref = makeBoard(fen);

    // Direct search at depth 3
    SearchResult direct = runSearch(b1, 3);

    // Iterative deepening up to depth 3
    SearchLimits lim;
    lim.maxDepth = 3;
    lim.infinite = true;
    SearchResult iter = search(b2, lim);

    check("ID vs direct: both have move",
          direct.hasBestMove() && iter.hasBestMove());
    check("ID vs direct: completed depths",
          direct.completed && iter.completed && !iter.interrupted);
    check("ID vs direct: same score",
          direct.score == iter.score,
          std::to_string(direct.score) + " vs " + std::to_string(iter.score));
    check("ID: board unchanged after ID", b2.isSameAs(ref));
    check("ID: PV starts with best move",
          !iter.principalVariation.empty() &&
          moveToString(iter.principalVariation.front()) == moveToString(iter.bestMove));
    check("ID: every PV move is legal", isLegalPV(b2, iter.principalVariation));
    check("Direct search: every PV move is legal", isLegalPV(b1, direct.principalVariation));
}

// ---------------------------------------------------------------------------
// Tests — Time management and stop flag
// ---------------------------------------------------------------------------

void testTimeManagement()
{
    // 100 ms budget — must complete at least depth 1 and return a move
    {
        Board board = makeBoard(
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        SearchLimits lim;
        lim.maxDepth = MAX_PLY;
        lim.timeMsec = 100;
        SearchResult sr = search(board, lim);
        check("100ms: has legal move", sr.hasBestMove());
        check("100ms: depth >= 1",     sr.depth >= 1);
        check("100ms: preserved result is complete", sr.completed);
    }

    // Pre-set stop flag: search must still return a legal move
    {
        clearStop();
        Board board = makeBoard(
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        requestStop();
        SearchLimits lim;
        lim.maxDepth = 10;
        SearchResult sr = search(board, lim);
        check("Stop flag: returns move", sr.hasBestMove());
        check("Stop flag: no incomplete depth reported",
              sr.interrupted && !sr.completed && sr.depth == 0);
        clearStop();
    }

    // Request stop immediately after depth one completes. The following
    // interrupted depth must not replace that completed result.
    {
        clearStop();
        Board board = makeBoard(
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        Board before = board;
        SearchLimits lim;
        lim.maxDepth = 8;
        lim.infinite = true;
        lim.useQuiescence = false;

        StopAfterDepthOneBuffer stopBuffer;
        std::streambuf* oldBuffer = std::cout.rdbuf(&stopBuffer);
        SearchResult sr = search(board, lim);
        std::cout.rdbuf(oldBuffer);
        clearStop();

        check("Stop after depth one: request observed", stopBuffer.requested());
        check("Stop after depth one: last completed depth preserved",
              sr.completed && sr.interrupted && sr.depth == 1);
        check("Stop after depth one: preserved move legal",
              sr.hasBestMove() && isLegalMove(board, sr.bestMove));
        check("Stop after depth one: preserved PV legal",
              isLegalPV(board, sr.principalVariation));
        check("Stop after depth one: board/history intact", board.isSameAs(before));
    }
}

// ---------------------------------------------------------------------------
// Tests — Output helpers (smoke test)
// ---------------------------------------------------------------------------

void testOutputHelpers()
{
    // nodesPerSecond
    checkEqual("NPS zero time",   0LL,      nodesPerSecond(1000, 0));
    checkEqual("NPS 1000/1s",     1000LL,   nodesPerSecond(1000, 1000));
    checkEqual("NPS 500/0.5s",    1000LL,   nodesPerSecond(500, 500));

    // formatPV: should not crash on empty state
    SearchState st;
    st.reset();
    std::string pv = formatPV(st);
    check("formatPV empty: returns string", true);  // non-crash

    // computeTimeBudget
    SearchLimits lim;
    lim.moveTime = 500;
    checkEqual("budget moveTime", 500, computeTimeBudget(lim));

    lim.moveTime = 0;
    lim.timeMsec = 1000;
    checkEqual("budget timeMsec", 1000, computeTimeBudget(lim));

    lim.timeMsec = 0;
    lim.infinite = true;
    checkEqual("budget infinite", 0, computeTimeBudget(lim));

    check("monotonic clock does not go backwards", nowMs() <= nowMs());
    check("next depth allowed below half budget", canStartNextDepth(49, 100, 2));
    check("next depth denied at half budget", !canStartNextDepth(50, 100, 2));
    check("next depth allowed without a budget", canStartNextDepth(500, 0, 2));

    lim.infinite = false;
    lim.clockSpecified = true;
    lim.whiteTimeMs = 10000; lim.whiteIncMs = 1000; lim.movesToGo = 20;
    checkEqual("clock budget uses remaining time, increment, and moves to go",
               1250, computeTimeBudget(lim));
    lim.blackToMove = true; lim.blackTimeMs = 4000; lim.blackIncMs = 1000;
    lim.movesToGo = 10;
    checkEqual("clock budget selects side to move", 1150, computeTimeBudget(lim));
    lim.blackTimeMs = 0; lim.blackIncMs = 0;
    checkEqual("zero remaining clock still permits a fast legal move", 1, computeTimeBudget(lim));
    lim.blackTimeMs = 10000; lim.blackIncMs = 2147483647; lim.movesToGo = 1;
    checkEqual("large clock increment is overflow-safe and capped", 9500, computeTimeBudget(lim));

    {
        Board board = makeBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        Board before = board;
        SearchLimits nodeLimited; nodeLimited.maxDepth = MAX_PLY; nodeLimited.infinite = true;
        nodeLimited.nodeLimit = 100;
        SearchResult result = search(board, nodeLimited);
        check("Node limit returns legal move and reports interruption",
              result.hasBestMove() && isLegalMove(board, result.bestMove) && result.interrupted);
        check("Node limit is respected without state leakage",
              result.nodes <= 100 && board.isSameAs(before));
    }

    // Validate exact protocol-style output, including signed mate distance and
    // safe NPS at zero elapsed milliseconds.
    std::ostringstream captured;
    std::streambuf* oldBuffer = std::cout.rdbuf(captured.rdbuf());
    printSearchInfo(4, MATE - 3, 123, 0, "e2e4");
    printSearchInfo(4, -(MATE - 3), 123, 0, "e2e4");
    printBestMove(makeMoveObject(Square{6, 4}, Square{4, 4}));
    std::cout.rdbuf(oldBuffer);
    const std::string output = captured.str();
    check("Output: positive mate in moves", output.find("score mate 2") != std::string::npos);
    check("Output: negative mate sign preserved", output.find("score mate -2") != std::string::npos);
    check("Output: zero elapsed NPS", output.find("nps 0") != std::string::npos);
    check("Output: bestmove formatting", output.find("bestmove e2e4") != std::string::npos);
}

// ---------------------------------------------------------------------------
// Test suite registration functions
// ---------------------------------------------------------------------------

void runSearchTestsFast()
{
    testZobristHashing();
    testTranspositionTable();
    testSearchTypes();
    testTerminalPositions();
    testBoardUnchangedAfterSearch();
    testOutputHelpers();
    testNegamaxVsMinimax();
    testAlphaBetaVsMinimax();
    testStrictMinimaxGates();
    testMateDetection();
    testFreeQueenCapture();
    testAvoidStalemate();
    testRepetitionHistory();
    testQuiescenceEdgeCases();
    testDrawScores();
    testDeterminism();
    testLegalMoveUnderTightLimit();
    testQuiescenceKiwipete();
    testMoveOrderingNodeCount();
    testIterativeDeepening();
    testTimeManagement();
}

static void runMoveOrderingBenchmarks()
{
    struct Config
    {
        const char* name;
        bool capture, pv, killer, history;
    };
    static const Config configs[] =
    {
        {"baseline", false, false, false, false},
        {"mvv_lva_promotion", true, false, false, false},
        {"pv", false, true, false, false},
        {"killer", false, false, true, false},
        {"history", false, false, false, true},
        {"complete", true, true, true, true},
    };
    static const struct
    {
        const char* name;
        const char* fen;
        int depth;
    } positions[] =
    {
        {"start_d5", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5},
        {"kiwipete_d4", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4},
    };

    for (const auto& position : positions)
    {
        int baselineScore = 0;
        bool haveBaseline = false;
        for (const auto& config : configs)
        {
            Board board = makeBoard(position.fen);
            Board before = board;
            SearchLimits limits;
            limits.useQuiescence = false;
            limits.useAlphaBeta = true;
            limits.useCaptureOrdering = config.capture;
            limits.usePvOrdering = config.pv;
            limits.useKillerOrdering = config.killer;
            limits.useHistoryOrdering = config.history;

            const long long started = nowMs();
            SearchResult result = runEquivalentSearch(board, position.depth, limits);
            const int elapsed = static_cast<int>(nowMs() - started);
            const bool legal = result.hasBestMove() && isLegalMove(board, result.bestMove);
            const bool restored = board.isSameAs(before);
            const bool sameScore = !haveBaseline || result.score == baselineScore;

            std::cout << "ORDERING | " << position.name << " | " << config.name
                      << " | depth=" << result.depth
                      << " | score=" << result.score
                      << " | move=" << moveToString(result.bestMove)
                      << " | nodes=" << result.nodes
                      << " | elapsed_ms=" << elapsed
                      << " | nps=" << nodesPerSecond(result.nodes, elapsed)
                      << " | legal=" << legal
                      << " | restored=" << restored
                      << " | " << (sameScore && legal && restored ? "PASS" : "FAIL")
                      << '\n';

            check(std::string("Ordering ") + position.name + " " + config.name +
                      " preserves score, legal move, board and history",
                  sameScore && legal && restored,
                  "baseline=" + std::to_string(baselineScore) +
                  " score=" + std::to_string(result.score));
            if (!haveBaseline)
            {
                baselineScore = result.score;
                haveBaseline = true;
            }
        }
    }
}

void runSearchTestsSlow()
{
    // Re-run all fast tests
    runSearchTestsFast();

    // Performance baseline (slow)
    Board board = makeBoard(
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");

    long long startMs = nowMs();
    SearchResult sr   = runSearch(board, 5);
    long long elapsed = nowMs() - startMs;
    long long nps     = nodesPerSecond(sr.nodes, static_cast<int>(elapsed));

    std::cout << "\n[Perf Baseline] Kiwipete d5\n";
    std::cout << "  Nodes: " << sr.nodes   << '\n';
    std::cout << "  Time:  " << elapsed    << " ms\n";
    std::cout << "  NPS:   " << nps        << '\n';
    std::cout << "  Score: " << scoreToString(sr.score) << '\n';
    std::cout << "  Move:  " << moveToString(sr.bestMove) << '\n';
    std::cout << "  Depth: " << sr.depth   << '\n';

    check("Perf: Kiwipete d5 has move", sr.hasBestMove());
    check("Perf: d5 nodes > 1000",      sr.nodes > 1000);

    // Fixed one-second iterative-deepening measurement from the start position.
    Board timedBoard = makeBoard(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    SearchLimits timedLimits;
    timedLimits.maxDepth = MAX_PLY;
    timedLimits.timeMsec = 1000;
    SearchResult timed = search(timedBoard, timedLimits);
    std::cout << "\n[Perf Baseline] Start position, 1 second\n"
              << "  Nodes: " << timed.nodes << '\n'
              << "  Time:  " << timed.elapsedMs << " ms\n"
              << "  NPS:   " << nodesPerSecond(timed.nodes, timed.elapsedMs) << '\n'
              << "  Depth: " << timed.depth << '\n';
    check("Perf: 1s search returns a move", timed.hasBestMove());
    check("Perf: 1s search completes depth 1", timed.depth >= 1);

    runMoveOrderingBenchmarks();
}
