
#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <chrono>
#include <iomanip>

#include "../engine/Board.h"
#include "../engine/legal.h"
#include "test_data.h"
#include "test_framework.h"

using namespace std;

// ============================================================
// 1. Standard Perft
// ============================================================

uint64_t perft(Board& board, int depth)
{
    if (depth == 0) {
        return 1;
    }

    vector<Move> moves = generateLegalMoves(board);
    uint64_t nodes = 0;

    for (const Move& move : moves) {
        board.makeMove(move);
        nodes += perft(board, depth - 1);
        board.undoMove();
    }

    return nodes;
}

// ============================================================
// 2. Bulk-counting Perft
// At depth 1, count legal moves directly.
// ============================================================

uint64_t perftBulk(Board& board, int depth)
{
    if (depth == 0) {
        return 1;
    }

    vector<Move> moves = generateLegalMoves(board);

    if (depth == 1) {
        return static_cast<uint64_t>(moves.size());
    }

    uint64_t nodes = 0;

    for (const Move& move : moves) {
        board.makeMove(move);
        nodes += perftBulk(board, depth - 1);
        board.undoMove();
    }

    return nodes;
}

// ============================================================
// 3. Perft Timing
// ============================================================

struct PerftTiming {
    uint64_t nodes;
    double seconds;
    double nodesPerSecond;
};

PerftTiming perftTimed(Board& board, int depth)
{
    const auto start = chrono::steady_clock::now();

    const uint64_t nodes = perftBulk(board, depth);

    const auto finish = chrono::steady_clock::now();

    const chrono::duration<double> elapsed = finish - start;
    const double seconds = elapsed.count();

    double nps = 0.0;

    if (seconds > 0.0) {
        nps = static_cast<double>(nodes) / seconds;
    }

    return {nodes, seconds, nps};
}

// ============================================================
// 4. Perft Divide
// ============================================================

void perftDivide(Board& board, int depth)
{
    if (depth <= 0) {
        cout << "Depth must be at least 1.\n";
        return;
    }

    vector<Move> moves = generateLegalMoves(board);
    uint64_t total = 0;

    for (const Move& move : moves) {
        board.makeMove(move);
        const uint64_t nodes = perft(board, depth - 1);
        board.undoMove();

        cout << squareToName(move.from)
             << squareToName(move.to);

        if (move.promotion != '.') {
            cout << move.promotion;
        }

        cout << ": " << nodes << '\n';
        total += nodes;
    }

    cout << "Total nodes: " << total << '\n';
}

// ============================================================
// 5. Fast and Slow Perft Tests
// ============================================================

static void runPerftTests(bool slowTests)
{
    int localPassed = 0;
    int localFailed = 0;

    cout << "\n========== "
         << (slowTests ? "SLOW" : "FAST")
         << " PERFT SUITE ==========\n";

    for (const PerftTestCase& test : perftTestCases) {
        // Fast suite: depths 1-3 for every position.
        if (!slowTests && test.depth > 3) {
            continue;
        }

        // Slow suite: depth 4 for every position,
        // and depth 5 for the starting position only.
        if (slowTests) {
            if (test.depth < 4) {
                continue;
            }

            if (test.depth == 5 &&
                test.name != "Starting Position") {
                continue;
            }
        }

        Board board;
        string error;

        const string testName =
            test.name + " - Depth " + to_string(test.depth);

        if (!board.loadFEN(test.fen, &error)) {
            TestFramework::check(
                testName + ": FEN loads successfully",
                false,
                "FEN: " + test.fen + " | Error: " + error
            );

            localFailed += 2;
            continue;
        }

        // Standard Perft result.
        const uint64_t standardNodes = perft(board, test.depth);

        TestFramework::checkEqual(
            testName,
            test.expectedNodes,
            standardNodes,
            "Standard Perft | FEN: " + test.fen
        );

        if (standardNodes == test.expectedNodes) {
            ++localPassed;
        } else {
            ++localFailed;

            cout << "\nDivide for " << test.name
                 << ", depth " << test.depth << ":\n";

            perftDivide(board, test.depth);
        }

        // Bulk Perft result.
        // The same board is used because perft must leave it unchanged.
        const uint64_t bulkNodes = perftBulk(board, test.depth);

        TestFramework::checkEqual(
            testName + " (Bulk)",
            test.expectedNodes,
            bulkNodes,
            "Bulk Perft | FEN: " + test.fen
        );

        if (bulkNodes == test.expectedNodes) {
            ++localPassed;
        } else {
            ++localFailed;
        }
    }

    cout << "\n========== "
         << (slowTests ? "SLOW" : "FAST")
         << " PERFT SUMMARY ==========\n";

    cout << "Passed: " << localPassed << '\n';
    cout << "Failed: " << localFailed << '\n';
}

// ============================================================
// 6. Performance Baseline
// Times the starting position at depth 5 using bulk Perft.
// ============================================================


void runPerftBenchmark()
{
    cout << "\n========== PERFT BENCHMARK ==========\n";
    cout << fixed << setprecision(3);

    int benchmarkCount = 0;

    for (const PerftTestCase& test : perftTestCases) {
        // Benchmark all depth-4 positions and the starting position at depth 5.
        const bool shouldBenchmark =
        (test.depth == 4 && test.name != "Starting Position") ||
        (test.name == "Starting Position" && test.depth == 5);

        if (!shouldBenchmark) {
            continue;
        }

        Board board;
        string error;

        const string testName =
            test.name + " - Depth " + to_string(test.depth);

        cout << "\nPosition: " << test.name << '\n';
        cout << "Depth: " << test.depth << '\n';

        if (!board.loadFEN(test.fen, &error)) {
            TestFramework::check(
                "Perft benchmark: " + testName + " FEN loads",
                false,
                "FEN: " + test.fen + " | Error: " + error
            );
            continue;
        }

        const PerftTiming result = perftTimed(board, test.depth);

        cout << "Expected nodes: " << test.expectedNodes << '\n';
        cout << "Actual nodes:   " << result.nodes << '\n';
        cout << "Elapsed time:   " << result.seconds << " seconds\n";
        cout << "Nodes/second:   "
             << setprecision(0) << result.nodesPerSecond
             << setprecision(3) << '\n';

        TestFramework::checkEqual(
            "Perft benchmark: " + testName,
            test.expectedNodes,
            result.nodes,
            "Perft node-count correctness"
        );

        ++benchmarkCount;
    }

    cout << "\nBenchmarked positions: " << benchmarkCount << '\n';
    cout << "====================================\n";
}

// ============================================================
// 7. Suite entry points
// ============================================================

void runPerftSuiteFast()
{
    runPerftTests(false);
}

void runPerftSuiteSlow()
{
    runPerftTests(true);
    runPerftBenchmark();
}
