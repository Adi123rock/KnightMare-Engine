#include "../engine2/search.h"
#include "../engine2/timer.h"
#include "../engine/legal.h"
#include <iostream>
#include <vector>
#include <string>
#include <iterator>

struct Position { const char* name; const char* fen; int depth; };
static const Position positions[] = {
    {"opening_start", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5},
    {"middlegame_kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4},
    {"tactical_hanging_queen", "rnb1kbnr/pppp1ppp/8/4p3/4P1q1/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3", 5},
    {"endgame_rook_ladder", "7k/8/8/8/8/8/8/RR5K w - - 0 1", 5},
    {"high_branching_position6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 4},
};

int main() {
    bool success = true;
    struct Config { const char* name; bool tt, pvs, aspiration; };
    const Config configs[] = {
        {"BASELINE", false, false, false},
        {"TT_ONLY", true, false, false},
        {"PVS_ONLY", false, true, false},
        {"ASPIRATION_ONLY", false, false, true},
        {"TT_PVS_ASPIRATION", true, true, true},
    };
    for (const auto& p : positions) {
        SearchResult baseline;
        for (std::size_t variant = 0; variant < std::size(configs); ++variant) {
            Board board;
            if (!board.loadFEN(p.fen)) return 2;
            const Board before = board;
            SearchLimits limits;
            limits.maxDepth = p.depth;
            limits.infinite = true;
            limits.useQuiescence = false;
            limits.useTranspositionTable = configs[variant].tt;
            limits.usePvs = configs[variant].pvs;
            limits.useAspiration = configs[variant].aspiration;
            globalTranspositionTable().clear();
            const auto began = nowMs();
            SearchResult result = search(board, limits);
            const int elapsed = static_cast<int>(nowMs() - began);
            const bool intact = board.isSameAs(before);
            const bool legal = !result.hasBestMove() || isLegalMove(board, result.bestMove);
            if (variant == 0) baseline = result;
            const bool scoreMatch = variant == 0 || result.score == baseline.score;
            success = success && intact && legal && scoreMatch && result.completed;
            std::cout << "PHASE8_BENCH | " << p.name << " | "
                      << configs[variant].name
                      << " | depth=" << result.depth
                      << " | nodes=" << result.nodes
                      << " | elapsed_ms=" << elapsed
                      << " | nps=" << nodesPerSecond(result.nodes, elapsed)
                      << " | score=" << result.score
                      << " | bestmove=" << (result.hasBestMove() ? moveToString(result.bestMove) : "0000")
                      << " | tt_hits=" << result.ttHits
                      << " | tt_cutoffs=" << result.ttCutoffs
                      << " | beta_cutoffs=" << result.betaCutoffs
                      << " | first_move_cutoffs=" << result.firstMoveCutoffs
                      << " | legal=" << legal << " | restored=" << intact
                      << " | score_match=" << scoreMatch << '\n';
        }
    }
    return success ? 0 : 1;
}
