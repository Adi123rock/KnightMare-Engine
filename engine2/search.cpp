// engine2/search.cpp
// Phase 7 — Complete search implementation.
//
// Conventions:
//   - All scores are from the side-to-move perspective.
//   - MATE = 30000; checkmate score = MATE - ply (for the winning side).
//   - The existing evaluate() already returns side-to-move perspective.
//   - positionKeys is pushed AFTER makeMove (so the key for the resulting
//     position is at positionKeys.back()), and popped on undoMove.

#include "search.h"
#include "timer.h"
#include "ordering.h"

#include "../engine/evaluation.h"   // evaluate(), evaluateWhite()
#include "../engine/legal.h"        // generateLegalMoves()
#include "../engine/rules.h"        // isInsufficientMaterial(), isFiftyMoveDraw(), isCheckmate(), isStalemate()
#include "../engine/attacks.h"      // isInCheck(), opposite()

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <cassert>
#include <cstdint>
#include <mutex>

// ---------------------------------------------------------------------------
// Internal colour helper
// ---------------------------------------------------------------------------
static Colour sideColour(const Board& board)
{
    return board.getSideToMove() == SideToMove::white ? Colour::white : Colour::black;
}

TranspositionTable& globalTranspositionTable() {
    static TranspositionTable table(16);
    return table;
}

// ---------------------------------------------------------------------------
// Draw detection during search
// ---------------------------------------------------------------------------

bool repetitionSeenBefore(const Board& board, int /*searchPly*/)
{
    const std::vector<std::uint64_t>& keys = board.getHashHistory();
    if (keys.size() < 3) return false;
    const std::uint64_t currentKey = board.getZobristHash();
    const std::vector<std::string>& canonicalKeys = board.getPositionKeys();
    std::string currentCanonical;
    bool canonicalReady = false;

    int halfmoveLimit = board.getHalfMove();
    int lastIdx = static_cast<int>(keys.size()) - 1;

    for (int i = lastIdx - 2, pliesBack = 2;
         i >= 0 && pliesBack <= halfmoveLimit;
         i -= 2, pliesBack += 2)
    {
        if (keys[i] == currentKey && static_cast<std::size_t>(i) < canonicalKeys.size()) {
            if (!canonicalReady) {
                currentCanonical = positionKey(board);
                canonicalReady = true;
            }
            if (canonicalKeys[static_cast<std::size_t>(i)] == currentCanonical)
                return true;   // Found one earlier occurrence — draw
        }
    }
    return false;
}

bool isDrawInSearch(const Board& board)
{
    if (isInsufficientMaterial(board)) return true;
    if (isFiftyMoveDraw(board))        return true;
    return false;
}

int terminalScore(bool inCheck, int ply)
{
    if (inCheck) return matedIn(ply);  // Checkmate: we are mated
    return 0;                           // Stalemate
}

static std::vector<Move> rootPV(const SearchState& state)
{
    std::vector<Move> pv;
    const int length = std::max(0, std::min(state.pvLength[0], MAX_PLY + 1));
    pv.reserve(static_cast<std::size_t>(length));
    for (int i = 0; i < length; ++i)
        pv.push_back(state.pvTable[0][i]);
    return pv;
}

// ---------------------------------------------------------------------------
// Time and stop management
// ---------------------------------------------------------------------------

void checkStop(SearchState& state, const SearchLimits& limits, long long startMs)
{
    if (state.stopped) return;
    if (limits.nodeLimit > 0 && static_cast<std::uint64_t>(state.nodes) >= limits.nodeLimit) {
        state.stopped = true;
        return;
    }

    // Respect explicit stop requests from outside (e.g. UCI "stop").
    if (isStopRequested())
    {
        state.stopped = true;
        return;
    }

    // Respect infinite mode.
    if (limits.infinite) return;

    // Check time budget.
    int budget = computeTimeBudget(limits);
    if (budget > 0)
    {
        long long elapsed = static_cast<long long>(nowMs()) - startMs;
        if (elapsed >= budget)
            state.stopped = true;
    }
}

int computeTimeBudget(const SearchLimits& limits)
{
    if (limits.infinite)  return 0;
    if (limits.moveTime > 0) return limits.moveTime;
    if (limits.timeMsec > 0) return limits.timeMsec;
    const int remaining = limits.blackToMove ? limits.blackTimeMs : limits.whiteTimeMs;
    const int increment = limits.blackToMove ? limits.blackIncMs : limits.whiteIncMs;
    if (limits.clockSpecified && remaining <= 0) return 1;
    if (remaining > 0) {
        const long long moves = limits.movesToGo > 0 ? limits.movesToGo : 30;
        const long long reserve = std::max(5LL, static_cast<long long>(remaining) / 20);
        const long long allocation = static_cast<long long>(remaining) / moves +
                                     std::max(0LL, static_cast<long long>(increment)) * 3 / 4;
        return static_cast<int>(std::max(1LL, std::min(allocation,
                                      static_cast<long long>(remaining) - reserve)));
    }
    return 0;
}

bool canStartNextDepth(int elapsed, int budget, int /*depth*/)
{
    if (budget <= 0) return true;  // No limit
    // Conservative: don't start if we've used more than 50% of the budget.
    return elapsed < budget / 2;
}

long long nodesPerSecond(long long nodes, int elapsedMs)
{
    if (elapsedMs <= 0) return 0;
    return nodes * 1000LL / static_cast<long long>(elapsedMs);
}

// ---------------------------------------------------------------------------
// Principal variation table
// ---------------------------------------------------------------------------

void updatePV(SearchState& state, int ply, const Move& move)
{
    // Triangular PV update:
    //   pvTable[ply][0] = move
    //   pvTable[ply][1..] = pvTable[ply+1][0..]
    state.pvTable[ply][0] = move;
    const int nextLen = (ply + 1 <= MAX_PLY) ? state.pvLength[ply + 1] : 0;
    for (int i = 0; i < nextLen && (ply + 1 + i) <= MAX_PLY; ++i)
        state.pvTable[ply][i + 1] = state.pvTable[ply + 1][i];
    state.pvLength[ply] = 1 + nextLen;
}

std::string formatPV(const SearchState& state)
{
    std::string result;
    int len = state.pvLength[0];
    for (int i = 0; i < len && i <= MAX_PLY; ++i)
    {
        if (i > 0) result += ' ';
        result += moveToString(state.pvTable[0][i]);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Output
// ---------------------------------------------------------------------------

void printSearchInfo(
    int        depth,
    int        score,
    long long  nodes,
    int        elapsedMs,
    const std::string& pvStr)
{
    long long nps = nodesPerSecond(nodes, elapsedMs);
    std::lock_guard<std::mutex> lock(g_protocolOutputMutex);
    std::cout << "info depth " << depth
              << " score " << (isMateScore(score)
                    ? "mate " + std::to_string(
                          (score > 0 ? 1 : -1) *
                          ((std::abs(mateDistance(score)) + 1) / 2))
                    : "cp " + std::to_string(score))
              << " nodes " << nodes
              << " time "  << elapsedMs
              << " nps "   << nps
              << " pv "    << pvStr
              << '\n' << std::flush;
}

void printBestMove(const Move& move)
{
    std::lock_guard<std::mutex> lock(g_protocolOutputMutex);
    std::cout << "bestmove " << moveToString(move) << '\n' << std::flush;
}

// ---------------------------------------------------------------------------
// Quiescence search
// ---------------------------------------------------------------------------

int quiesce(
    Board&        board,
    SearchState&  state,
    const SearchLimits& limits,
    long long     startMs,
    int           alpha,
    int           beta,
    int           qply,
    int           rootPly)
{
    // Respect stop
    if (state.stopped) return 0;

    // Update seldepth
    int totalPly = rootPly + qply;
    if (totalPly > state.seldepth) state.seldepth = totalPly;

    // Check stop periodically
    if (limits.nodeLimit > 0 && static_cast<std::uint64_t>(state.nodes) >= limits.nodeLimit) {
        state.stopped = true;
        return 0;
    }
    ++state.nodes;
    if ((state.nodes & (TIME_CHECK_INTERVAL - 1)) == 0)
        checkStop(state, limits, startMs);
    if (state.stopped) return 0;

    Colour us = sideColour(board);
    bool inCheck = isInCheck(board, us);

    // Generate legal moves
    std::vector<Move> legalMoves = generateLegalMoves(board);

    // Terminal position
    if (legalMoves.empty())
        return terminalScore(inCheck, totalPly);

    // Checkmate takes precedence over draw rules, including at qsearch nodes.
    if (totalPly > 0 && repetitionSeenBefore(board, totalPly)) return 0;
    if (totalPly > 0 && isDrawInSearch(board)) return 0;

    // At the hard qsearch bound, still resolve check without stand-pat. Make
    // one final pass over legal evasions and statically score their positions.
    if (qply >= MAX_QPLY || totalPly >= MAX_PLY + MAX_QPLY)
    {
        if (!inCheck) return evaluate(board);
        int best = -INF;
        for (const Move& move : legalMoves)
        {
            board.makeMove(move);
            int score = -evaluate(board);
            board.undoMove();
            if (score > best) best = score;
        }
        return best;
    }

    int standPat = 0;

    if (!inCheck)
    {
        // Stand-pat evaluation — only allowed when not in check
        standPat = evaluate(board);

        if (standPat >= beta)  return standPat;   // Fail-soft
        if (standPat > alpha)  alpha = standPat;
    }

    // Filter to tactical moves only (captures + promotions) unless in check
    std::vector<Move> searchMoves;
    searchMoves.reserve(legalMoves.size());

    if (inCheck)
    {
        // Stand-pat is invalid in check. Search every legal evasion until the
        // quiescence ply safeguard is reached.
        searchMoves = legalMoves;
    }
    else
    {
        // Out of check: captures and promotions only
        for (const Move& m : legalMoves)
        {
            if (isCapture(m) || isEnPassant(m) || isPromotion(m))
                searchMoves.push_back(m);
        }
    }

    if (searchMoves.empty())
    {
        // No tactical moves and not in check: return stand-pat
        return standPat;
    }

    // Order moves (PV move = null for quiescence)
    Move nullMove{};
    orderMoves(searchMoves, board, state, totalPly, nullMove, limits);

    int bestScore = inCheck ? matedIn(totalPly) : standPat;

    for (const Move& move : searchMoves)
    {
        board.makeMove(move);
        int score = -quiesce(board, state, limits, startMs, -beta, -alpha, qply + 1, rootPly);
        board.undoMove();

        if (state.stopped) return bestScore;

        if (score > bestScore) bestScore = score;
        if (score > alpha)
        {
            alpha = score;
            if (alpha >= beta) break;   // Beta cut-off
        }
    }

    return bestScore;
}

// ---------------------------------------------------------------------------
// Alpha-beta negamax
// ---------------------------------------------------------------------------

int alphaBeta(
    Board&        board,
    SearchState&  state,
    const SearchLimits& limits,
    long long     startMs,
    int           alpha,
    int           beta,
    int           depth,
    int           ply)
{
    // Respect stop
    if (state.stopped) return 0;

    // Update seldepth
    if (ply > state.seldepth) state.seldepth = ply;

    // PV length initialisation at this ply
    if (ply <= MAX_PLY) state.pvLength[ply] = 0;

    // Check stop periodically
    if (limits.nodeLimit > 0 && static_cast<std::uint64_t>(state.nodes) >= limits.nodeLimit) {
        state.stopped = true;
        return 0;
    }
    ++state.nodes;
    if ((state.nodes & (TIME_CHECK_INTERVAL - 1)) == 0)
        checkStop(state, limits, startMs);
    if (state.stopped) return 0;

    // At leaf nodes, drop into quiescence search (or static evaluation if disabled)
    if (depth <= 0)
    {
        if (limits.useQuiescence)
            return quiesce(board, state, limits, startMs, alpha, beta, 0, ply);
        return evaluate(board);
    }

    // Max ply safeguard
    if (ply >= MAX_PLY)
        return evaluate(board);

    Colour us = sideColour(board);
    bool inCheck = isInCheck(board, us);

    // Generate legal moves
    std::vector<Move> legalMoves = generateLegalMoves(board);

    // Terminal position
    if (legalMoves.empty())
        return terminalScore(inCheck, ply);

    // Checkmate takes precedence over claimable and automatic draw checks.
    if (ply > 0 && repetitionSeenBefore(board, ply)) return 0;
    if (ply > 0 && isDrawInSearch(board)) return 0;

    const int alphaOriginal = alpha;
    const int betaOriginal = beta;
    TTEntry* ttEntry = nullptr;
    Move ttMove{};
    if (limits.useTranspositionTable && state.tt != nullptr) {
        ttEntry = state.tt->probe(board.getZobristHash());
        if (ttEntry != nullptr) {
            ttMove = ttEntry->bestMove;
            if (ttEntry->depth >= depth && ttEntry->halfmoveClock == board.getHalfMove() &&
                ttEntry->quiescence == limits.useQuiescence) {
                const int cached = denormalizeMateScoreFromTT(ttEntry->score, ply);
                if (ttEntry->bound == TTBound::exact ||
                    (ttEntry->bound == TTBound::lower && cached >= beta) ||
                    (ttEntry->bound == TTBound::upper && cached <= alpha)) {
                    state.tt->recordCutoff();
                    return cached;
                }
            }
        }
    }

    // PV move for ordering: use stored PV from previous iteration
    Move pvMove{};
    if (ply < MAX_PLY && state.pvLength[ply] > 0)
        pvMove = state.pvTable[ply][0];
    else if (ttEntry != nullptr)
        pvMove = ttMove;

    // Order moves
    orderMoves(legalMoves, board, state, ply, pvMove, limits);

    int bestScore = -INF;
    Move bestMove{};

    int moveIndex = 0;
    for (const Move& move : legalMoves)
    {
        board.makeMove(move);
        int score;
        if (limits.useAlphaBeta && limits.usePvs && moveIndex > 0) {
            // PVS tests later moves with a null window, then restores a full
            // window only when a move improves alpha without already cutting beta.
            score = -alphaBeta(board, state, limits, startMs, -alpha - 1, -alpha, depth - 1, ply + 1);
            if (!state.stopped && score > alpha && score < beta)
                score = -alphaBeta(board, state, limits, startMs, -beta, -alpha, depth - 1, ply + 1);
        } else {
            const int childAlpha = limits.useAlphaBeta ? -beta : -INF;
            const int childBeta = limits.useAlphaBeta ? -alpha : INF;
            score = -alphaBeta(board, state, limits, startMs, childAlpha, childBeta, depth - 1, ply + 1);
        }
        board.undoMove();

        if (state.stopped) return bestScore;

        if (score > bestScore)
        {
            bestScore = score;
            bestMove  = move;
        }

        if (score > alpha)
        {
            alpha = score;
            updatePV(state, ply, move);

            if (limits.useAlphaBeta && alpha >= beta)
            {
                ++state.betaCutoffs;
                if (moveIndex == 0) ++state.firstMoveCutoffs;
                // Beta cut-off: update move ordering heuristics
                if (limits.useKillerOrdering)
                    updateKillers(state, ply, move);
                if (limits.useHistoryOrdering)
                    updateHistory(state, move, board.getSideToMove(), depth);
                break;
            }
        }
        ++moveIndex;
    }

    if (limits.useTranspositionTable && state.tt != nullptr && !state.stopped) {
        const TTBound bound = bestScore <= alphaOriginal ? TTBound::upper :
                              bestScore >= betaOriginal ? TTBound::lower : TTBound::exact;
        state.tt->store(board.getZobristHash(), depth, normalizeMateScoreForTT(bestScore, ply),
                        bound, bestMove, board.getHalfMove(), limits.useQuiescence);
    }

    return bestScore;
}

// ---------------------------------------------------------------------------
// Root search
// ---------------------------------------------------------------------------

std::vector<Move> generateRootMoves(Board& board)
{
    return generateLegalMoves(board);
}

static SearchResult searchRootWindow(
    Board&              board,
    SearchState&        state,
    const SearchLimits& limits,
    long long           startMs,
    int                 depth,
    int                 alpha,
    int                 beta)
{
    SearchResult result;
    result.depth = depth;

    std::vector<Move> rootMoves = generateRootMoves(board);
    if (rootMoves.empty())
    {
        // Terminal position — no move to return
        Colour us = sideColour(board);
        bool inCheck = isInCheck(board, us);
        result.score = terminalScore(inCheck, 0);
        result.completed = true;
        return result;
    }

    // Draw rules also apply at the root. Keep a legal move available to the
    // caller while reporting the drawn score.
    if (isDrawInSearch(board) || repetitionSeenBefore(board, 0))
    {
        result.bestMove = rootMoves.front();
        result.score = 0;
        result.nodes = state.nodes;
        result.completed = true;
        return result;
    }

    // Use PV move from previous iteration for root ordering
    Move pvMove{};
    if (state.pvLength[0] > 0)
        pvMove = state.pvTable[0][0];
    else if (limits.useTranspositionTable && state.tt != nullptr) {
        if (TTEntry* rootEntry = state.tt->probe(board.getZobristHash()))
            pvMove = rootEntry->bestMove;
    }

    orderMoves(rootMoves, board, state, 0, pvMove, limits);

    // Reset PV at root
    state.pvLength[0] = 0;

    const int alphaOriginal = alpha;
    const int betaOriginal = beta;
    int bestScore = -INF;
    Move bestMove{};

    int moveIndex = 0;
    for (const Move& move : rootMoves)
    {
        board.makeMove(move);
        int score;
        if (limits.useAlphaBeta && limits.usePvs && moveIndex > 0) {
            // Root PVS follows the same null-window/re-search rule as interior nodes.
            score = -alphaBeta(board, state, limits, startMs, -alpha - 1, -alpha, depth - 1, 1);
            if (!state.stopped && score > alpha && score < beta)
                score = -alphaBeta(board, state, limits, startMs, -beta, -alpha, depth - 1, 1);
        } else {
            const int childAlpha = limits.useAlphaBeta ? -beta : -INF;
            const int childBeta = limits.useAlphaBeta ? -alpha : INF;
            score = -alphaBeta(board, state, limits, startMs, childAlpha, childBeta, depth - 1, 1);
        }
        board.undoMove();

        if (state.stopped)
        {
            result.interrupted = true;
            result.nodes = state.nodes;
            result.elapsedMs = static_cast<int>(nowMs() - startMs);
            return result;
        }

        if (score > bestScore)
        {
            bestScore = score;
            bestMove  = move;

            // Update PV at root
            updatePV(state, 0, move);
        }

        if (score > alpha)
        {
            alpha = score;
        }
        if (limits.useAlphaBeta && alpha >= beta) {
            ++state.betaCutoffs;
            if (moveIndex == 0) ++state.firstMoveCutoffs;
            break;
        }
        ++moveIndex;
    }

    result.bestMove = bestMove;
    result.score    = bestScore;
    result.nodes    = state.nodes;
    result.elapsedMs = static_cast<int>(nowMs() - startMs);
    result.principalVariation = rootPV(state);
    result.completed = true;
    result.betaCutoffs = state.betaCutoffs;
    result.firstMoveCutoffs = state.firstMoveCutoffs;
    if (state.tt != nullptr) {
        result.ttHits = state.tt->stats().hits;
        result.ttCutoffs = state.tt->stats().cutoffs;
        const TTBound bound = bestScore <= alphaOriginal ? TTBound::upper :
                              bestScore >= betaOriginal ? TTBound::lower : TTBound::exact;
        state.tt->store(board.getZobristHash(), depth, normalizeMateScoreForTT(bestScore, 0),
                        bound, bestMove, board.getHalfMove(), limits.useQuiescence);
    }

    return result;
}

SearchResult searchRoot(Board& board, SearchState& state, const SearchLimits& limits,
                        long long startMs, int depth) {
    return searchRootWindow(board, state, limits, startMs, depth, -INF, INF);
}

// ---------------------------------------------------------------------------
// Iterative deepening
// ---------------------------------------------------------------------------

SearchResult iterativeDeepening(Board& board, const SearchLimits& limits)
{
    long long startMs = nowMs();

    SearchState state;
    state.reset();
    state.tt = (limits.useTranspositionTable && limits.useAlphaBeta)
        ? &globalTranspositionTable() : nullptr;
    if (state.tt != nullptr) state.tt->newSearch();

    int maxDepth = std::max(1, std::min(limits.maxDepth, MAX_PLY));
    int budget   = computeTimeBudget(limits);

    SearchResult lastCompleted;
    lastCompleted.bestMove = Move{};

    // Ensure we always have a legal move to return
    std::vector<Move> rootMoves = generateRootMoves(board);
    if (rootMoves.empty())
    {
        const bool inCheck = isInCheck(board, sideColour(board));
        lastCompleted.score = terminalScore(inCheck, 0);
        lastCompleted.completed = true;
        lastCompleted.elapsedMs = static_cast<int>(nowMs() - startMs);
        return lastCompleted;
    }

    lastCompleted.bestMove = rootMoves[0];
    if (isDrawInSearch(board) || repetitionSeenBefore(board, 0))
    {
        lastCompleted.score = 0;
        lastCompleted.completed = true;
        lastCompleted.elapsedMs = static_cast<int>(nowMs() - startMs);
        return lastCompleted;
    }

    for (int depth = 1; depth <= maxDepth; ++depth)
    {
        if (isStopRequested())
        {
            state.stopped = true;
            lastCompleted.interrupted = true;
            break;
        }

        state.stopped = false;

        SearchResult r;
        if (limits.useAspiration && limits.useAlphaBeta && depth >= 2 && lastCompleted.depth > 0) {
            // A prior iteration's score seeds a narrow root window; fail-low/high
            // retries widen it geometrically until the result is inside the window.
            int window = 50;
            int alpha = std::max(-INF, lastCompleted.score - window);
            int beta = std::min(INF, lastCompleted.score + window);
            while (true) {
                r = searchRootWindow(board, state, limits, startMs, depth, alpha, beta);
                if (state.stopped || (r.score > alpha && r.score < beta)) break;
                // Widen the aspiration window geometrically after a fail-low/high.
                window = std::min(INF, window * 2);
                alpha = std::max(-INF, lastCompleted.score - window);
                beta = std::min(INF, lastCompleted.score + window);
            }
        } else {
            r = searchRoot(board, state, limits, startMs, depth);
        }

        int elapsed = static_cast<int>(nowMs() - startMs);

        if (state.stopped)
        {
            // Never replace the last completed result with a partial depth.
            lastCompleted.interrupted = true;
            lastCompleted.nodes = state.nodes;
            lastCompleted.elapsedMs = elapsed;
            break;
        }

        r.depth     = depth;
        r.nodes     = state.nodes;
        r.elapsedMs = elapsed;
        r.completed = true;
        r.interrupted = false;
        r.betaCutoffs = state.betaCutoffs;
        r.firstMoveCutoffs = state.firstMoveCutoffs;
        if (state.tt != nullptr) {
            r.ttHits = state.tt->stats().hits;
            r.ttCutoffs = state.tt->stats().cutoffs;
        }

        lastCompleted = r;

        // Print info
        std::string pvStr = formatPV(state);
        printSearchInfo(depth, r.score, state.nodes, elapsed, pvStr);

        // Early termination on forced mate
        if (isMateScore(r.score))
        {
            int dist = mateDistance(r.score);
            // If we found a mate within the remaining depth, stop early
            if (r.score > 0 && dist <= depth)
                break;
        }

        // Time management: can we afford another iteration?
        if (budget > 0 && !canStartNextDepth(elapsed, budget, depth + 1))
            break;
    }

    return lastCompleted;
}

// ---------------------------------------------------------------------------
// Top-level entry point
// ---------------------------------------------------------------------------

SearchResult search(Board& board, const SearchLimits& limits)
{
    return iterativeDeepening(board, limits);
}
