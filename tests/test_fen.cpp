
#include <iostream>
#include <string>
#include <vector>

#include "../engine/Board.h"
#include "test_framework.h"
#include "test_data.h"

using namespace std;
using TestFramework::check;
using TestFramework::checkEqual;
using TestFramework::checkFen;

void testFenRoundTrip(const FenTestCase& test)
{
    Board board;
    string error;

    const bool loaded = board.loadFEN(test.fen, &error);

    check(
        loaded,
        test.name + ": FEN loads successfully",
        loaded ? "" : "FEN: " + test.fen + " | Parser error: " + error
    );

    if (!loaded)
        return;

    checkFen(
        test.name + ": FEN round trip preserves all six fields",
        board,
        test.fen
    );
}

void testFenRoundTripMany()
{
    cout << "\n--- FEN Round-Trip Tests ---\n";

    for (const FenTestCase& test : fenRoundTripCases)
        testFenRoundTrip(test);
}

void testLoadResetsHistory()
{
    cout << "\n--- FEN History Reset Tests ---\n";

    Board board;
    string error;

    const bool loaded = board.loadFEN(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        &error
    );

    check(loaded, "Load starting position for history test");

    if (!loaded)
        return;

    checkEqual(
        "FEN load leaves undo history empty",
        0,
        board.historySize()
    );

    checkEqual(
        "FEN load records exactly one repetition position",
        size_t{1},
        board.getPositionKeys().size()
    );

    // Make one move to populate undo history.
    const Move move = makeMoveObject(
        Square{7, 6}, // g1
        Square{5, 5}  // f3
    );

    board.makeMove(move);

    checkEqual(
        "Making g1f3 adds one undo-history entry",
        1,
        board.historySize(),
        "Move: g1f3 | FEN: " + board.toFEN()
    );

    // Load a different position.
    const bool loadedAnother = board.loadFEN(
        "4k3/8/8/8/8/8/8/4K3 w - - 0 1",
        &error
    );

    check(loadedAnother, "Load another FEN after a move");

    if (!loadedAnother)
        return;

    checkEqual(
        "Successful FEN load clears undo history",
        0,
        board.historySize()
    );

    checkEqual(
        "Successful FEN load resets repetition history to one key",
        size_t{1},
        board.getPositionKeys().size()
    );
}

void testFenRejects(const string& badFen, const string& description)
{
    Board board;
    string error;

    const bool loaded = board.loadFEN(badFen, &error);

    check(
        !loaded,
        description + ": invalid FEN is rejected",
        "FEN: " + badFen
    );

    check(
        !loaded || !error.empty(),
        description + ": parser provides an error message",
        "FEN: " + badFen
    );
}

void testFenRejectionSuite()
{
    cout << "\n--- Invalid FEN Rejection Tests ---\n";

    testFenRejects(
        "8/8/8/8/8/8/8/8 w - - 0",
        "Missing field"
    );

    testFenRejects(
        "8/8/8/8/8/8/8/8 w - - 0 1 extra",
        "Extra field"
    );

    testFenRejects(
        "8/8/8/8/8/8/8 w - - 0 1",
        "Seven ranks"
    );

    testFenRejects(
        "8/8/8/8/8/8/8/9 w - - 0 1",
        "Nine squares in a rank"
    );

    testFenRejects(
        "8/8/8/8/8/8/8/4X2K w - - 0 1",
        "Invalid piece character"
    );

    testFenRejects(
        "8/8/8/8/8/8/8/8 x - - 0 1",
        "Invalid side to move"
    );

    testFenRejects(
        "8/8/8/8/8/8/8/8 w X - 0 1",
        "Invalid castling rights"
    );

    testFenRejects(
        "8/8/8/8/8/8/8/8 w - z9 0 1",
        "Invalid en passant square"
    );

    testFenRejects(
        "8/8/8/8/8/8/8/8 w - - -1 1",
        "Negative halfmove clock"
    );

    testFenRejects(
        "8/8/8/8/8/8/8/8 w - - abc 1",
        "Non-numeric halfmove clock"
    );

    testFenRejects(
        "8/8/8/8/8/8/8/8 w - - 0 xyz",
        "Non-numeric fullmove number"
    );
}

void testFailedLoadKeepsBoard()
{
    cout << "\n--- Failed FEN Load Safety Tests ---\n";

    Board board;
    string error;

    const bool loaded = board.loadFEN(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        &error
    );

    check(loaded, "Load initial board for failure-safety test");

    if (!loaded)
        return;

    // Make a move so that both histories contain useful state.
    board.makeMove(
        makeMoveObject(Square{7, 6}, Square{5, 5})
    );

    const string originalFen = board.toFEN();
    const int originalHistorySize = board.historySize();
    const vector<string> originalKeys = board.getPositionKeys();

    error.clear();

    const bool badLoad = board.loadFEN(
        "8/8/8/8/8/8/8/9 w - - 0 1",
        &error
    );

    check(!badLoad, "Malformed FEN load fails");

    check(
        !error.empty(),
        "Failed FEN load provides an error message"
    );

    checkFen(
        "Failed load preserves the board position",
        board,
        originalFen
    );

    checkEqual(
        "Failed load preserves undo history",
        originalHistorySize,
        board.historySize(),
        "Move before failed load: g1f3"
    );

    check(
        board.getPositionKeys() == originalKeys,
        "Failed load preserves repetition history",
        "FEN before failed load: " + originalFen
    );
}

void runFenTests()
{
    testFenRoundTripMany();
    testLoadResetsHistory();
    testFenRejectionSuite();
    testFailedLoadKeepsBoard();
}
