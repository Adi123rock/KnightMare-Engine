#include "../engine/Board.h"
#include "test_data.h"
#include "test_framework.h"
#include <iostream>
#include <chrono>
#include <vector>

std::uint64_t perftBulk(Board& board, int depth);
struct PerftTiming { std::uint64_t nodes; double seconds; double nodesPerSecond; };
PerftTiming perftTimed(Board& board, int depth);

int main() {
    struct Case { const char* name; const char* fen; int depth; std::uint64_t expected; };
    const Case cases[] = {
        {"Starting Position", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5, 4865609},
        {"Kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4, 4085603},
        {"Position 3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 4, 43238},
        {"Position 4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4, 422333},
        {"Position 5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4, 2103487},
        {"Position 6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 4, 3894594},
    };
    bool ok = true;
    for (const auto& c : cases) {
        Board board;
        if (!board.loadFEN(c.fen)) return 2;
        const auto start = std::chrono::steady_clock::now();
        const std::uint64_t nodes = perftBulk(board, c.depth);
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        const bool countOK = nodes == c.expected;
        ok = ok && countOK;
        std::cout << "PERFT_BENCH | " << c.name << " | depth=" << c.depth
                  << " | nodes=" << nodes << " | elapsed_ms=" << static_cast<long long>(ms)
                  << " | nps=" << static_cast<long long>(nodes * 1000.0 / (ms > 0 ? ms : 1))
                  << " | verified=" << countOK << '\n';
    }
    return ok ? 0 : 1;
}
