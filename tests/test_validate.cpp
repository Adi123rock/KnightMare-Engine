
#include <iostream>
#include <string>

#include "../engine/Board.h"
#include "../engine/validate.h"
#include "test_framework.h"

using namespace std;
using TestFramework::check;

void testPositionValidation()
{
    cout << "\n--- Position Validation Tests ---\n";

    Board board;
    string error;

    // A normal starting position should pass.
    check(
        board.loadFEN(
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            &error
        ),
        "Load valid starting position"
    );

    check(
        validatePosition(board, &error),
        "Valid starting position passes validation"
    );

    // Missing black king.
    board.loadFEN("8/8/8/8/8/8/8/4K3 w - - 0 1");

    check(
        !validatePosition(board, &error) && !error.empty(),
        "Missing black king is rejected"
    );

    // Adjacent kings.
    board.loadFEN("8/8/8/8/8/8/4k3/4K3 w - - 0 1");

    check(
        !validatePosition(board, &error) && !error.empty(),
        "Adjacent kings are rejected"
    );

    // White pawn on rank 8.
    board.loadFEN("P3k3/8/8/8/8/8/8/4K3 w - - 0 1");

    check(
        !validatePosition(board, &error) && !error.empty(),
        "Pawn on rank 8 is rejected"
    );

    // Nine white pawns.
    board.loadFEN("4k3/8/8/8/8/8/PPPPPPPP/PP2K3 w - - 0 1");

    check(
        !validatePosition(board, &error) && !error.empty(),
        "More than eight white pawns are rejected"
    );

    // White is in check, but black is to move.
    board.loadFEN("4k3/8/8/8/8/8/4r3/4K3 b - - 0 1");

    check(
        !validatePosition(board, &error) && !error.empty(),
        "Side not to move cannot be in check"
    );

    // Castling right but missing white kingside rook.
    board.loadFEN("4k3/8/8/8/8/8/8/4K3 w K - 0 1");

    check(
        !validatePosition(board, &error) && !error.empty(),
        "Castling right without required rook is rejected"
    );

    // White to move, target c6, but the black pawn is missing from c5.
    board.loadFEN("4k3/8/8/8/8/8/8/4K3 w - c6 0 1");

    check(
        !validatePosition(board, &error) && !error.empty(),
        "Inconsistent en-passant target is rejected"
    );
}

void runValidationTests()
{
    testPositionValidation();
}
