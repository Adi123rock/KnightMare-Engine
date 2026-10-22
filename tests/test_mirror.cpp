
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <cstdint>

#include "../engine/Board.h"
#include "../engine/legal.h"
#include "test_data.h"
#include "test_framework.h"

// Implemented in tests/perft.cpp.
uint64_t perftBulk(Board& board, int depth);

// Mirrors a FEN vertically and swaps piece colours.
std::string mirrorFen(const std::string& fen)
{
    std::istringstream input(fen);

    std::string placement;
    std::string sideToMove;
    std::string castling;
    std::string enPassant;
    std::string halfmove;
    std::string fullmove;
    std::string extra;

    if (!(input >> placement >> sideToMove >> castling
                >> enPassant >> halfmove >> fullmove)) {
        return "";
    }

    // Reject unexpected extra fields.
    if (input >> extra) {
        return "";
    }

    // Split the board placement into its eight ranks.
    std::vector<std::string> ranks;
    std::stringstream rankStream(placement);
    std::string rank;

    while (std::getline(rankStream, rank, '/')) {
        ranks.push_back(rank);
    }

    if (ranks.size() != 8) {
        return "";
    }

    // Reverse rank order and swap the colour of every piece.
    std::string mirroredPlacement;

    for (int row = 7; row >= 0; --row) {
        for (char piece : ranks[row]) {
            if (piece >= 'A' && piece <= 'Z') {
                mirroredPlacement +=
                    static_cast<char>(piece - 'A' + 'a');
            } else if (piece >= 'a' && piece <= 'z') {
                mirroredPlacement +=
                    static_cast<char>(piece - 'a' + 'A');
            } else {
                mirroredPlacement += piece;
            }
        }

        if (row > 0) {
            mirroredPlacement += '/';
        }
    }

    // Swap the side to move.
    if (sideToMove == "w") {
        sideToMove = "b";
    } else if (sideToMove == "b") {
        sideToMove = "w";
    } else {
        return "";
    }

    // Swap castling rights between the two colours.
    if (castling != "-") {
        std::string mirroredCastling;

        for (char right : castling) {
            switch (right) {
                case 'K': mirroredCastling += 'k'; break;
                case 'Q': mirroredCastling += 'q'; break;
                case 'k': mirroredCastling += 'K'; break;
                case 'q': mirroredCastling += 'Q'; break;
                default: return "";
            }
        }

        // Return rights in standard KQkq order.
        const std::string order = "KQkq";
        std::string sortedCastling;

        for (char right : order) {
            if (mirroredCastling.find(right) != std::string::npos) {
                sortedCastling += right;
            }
        }

        castling = sortedCastling.empty() ? "-" : sortedCastling;
    }

    // Mirror the en-passant target's rank.
    // Files stay unchanged: e3 becomes e6, for example.
    if (enPassant != "-") {
        if (enPassant.size() != 2 ||
            enPassant[0] < 'a' || enPassant[0] > 'h' ||
            enPassant[1] < '1' || enPassant[1] > '8') {
            return "";
        }

        enPassant[1] =
            static_cast<char>('9' - enPassant[1]);
    }

    // Halfmove and fullmove counters are retained.
    return mirroredPlacement + " " + sideToMove + " " +
           castling + " " + enPassant + " " +
           halfmove + " " + fullmove;
}

// Checks FEN transformation and verifies that mirroring twice
// restores the original FEN.
void runPerftMirrorFast()
{
    std::cout << "\n========== FEN MIRROR TESTS ==========\n";

    for (const FenTestCase& test : fenRoundTripCases) {
        const std::string mirrored = mirrorFen(test.fen);
        const std::string restored = mirrorFen(mirrored);

        TestFramework::check(
            test.name + ": mirrored FEN is valid",
            !mirrored.empty(),
            "Original FEN: " + test.fen
        );

        TestFramework::checkEqual(
            test.name + ": mirroring twice restores FEN",
            test.fen,
            restored
        );
    }

    std::cout << "\n========== PERFT MIRROR FAST ==========\n";

    for (const PerftTestCase& test : perftTestCases) {
        if (test.depth > 3) {
            continue;
        }

        Board originalBoard;
        Board mirroredBoard;
        std::string error;

        if (!originalBoard.loadFEN(test.fen, &error)) {
            TestFramework::check(
                test.name + " original FEN loads",
                false,
                error
            );
            continue;
        }

        const std::string mirrored = mirrorFen(test.fen);

        if (mirrored.empty() ||
            !mirroredBoard.loadFEN(mirrored, &error)) {
            TestFramework::check(
                test.name + " mirrored FEN loads",
                false,
                "Mirrored FEN: " + mirrored +
                " | Error: " + error
            );
            continue;
        }

        const uint64_t originalNodes =
            perftBulk(originalBoard, test.depth);

        const uint64_t mirroredNodes =
            perftBulk(mirroredBoard, test.depth);

        TestFramework::checkEqual(
            test.name + " mirror symmetry - Depth " +
                std::to_string(test.depth),
            originalNodes,
            mirroredNodes,
            "Original FEN: " + test.fen +
            " | Mirrored FEN: " + mirrored
        );
    }
}

// Deeper symmetry checks for the slow suite.
void runPerftMirrorSlow()
{
    std::cout << "\n========== PERFT MIRROR SLOW ==========\n";

    for (const PerftTestCase& test : perftTestCases) {
        if (test.depth < 4) {
            continue;
        }

        Board originalBoard;
        Board mirroredBoard;
        std::string error;

        if (!originalBoard.loadFEN(test.fen, &error)) {
            TestFramework::check(
                test.name + " original FEN loads",
                false,
                error
            );
            continue;
        }

        const std::string mirrored = mirrorFen(test.fen);

        if (mirrored.empty() ||
            !mirroredBoard.loadFEN(mirrored, &error)) {
            TestFramework::check(
                test.name + " mirrored FEN loads",
                false,
                "Mirrored FEN: " + mirrored +
                " | Error: " + error
            );
            continue;
        }

        const uint64_t originalNodes =
            perftBulk(originalBoard, test.depth);

        const uint64_t mirroredNodes =
            perftBulk(mirroredBoard, test.depth);

        TestFramework::checkEqual(
            test.name + " mirror symmetry - Depth " +
                std::to_string(test.depth),
            originalNodes,
            mirroredNodes,
            "Original FEN: " + test.fen +
            " | Mirrored FEN: " + mirrored
        );
    }
}
