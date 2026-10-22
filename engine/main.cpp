
#include <bits/stdc++.h>

#include "Board.h"
#include "MoveGenerator.h"
#include "attacks.h"
#include "legal.h"
#include "rules.h"

using namespace std;

// Perft functions implemented in tests/perft.cpp.
uint64_t perft(Board& board, int depth);
void perftDivide(Board& board, int depth);
void runPerftSuite();

int passed = 0;
int failed = 0;

// ============================================================
// TEST HELPERS
// ============================================================

void check(bool condition, const string& name)
{
    if (condition)
    {
        cout << "[PASS] " << name << '\n';
        passed++;
    }
    else
    {
        cout << "[FAIL] " << name << '\n';
        failed++;
    }
}

bool sameSquare(Square a, Square b)
{
    return a.r == b.r && a.c == b.c;
}

Square getSquare(const string& name)
{
    Square sq;

    if (!nameToSquare(name, sq))
        throw invalid_argument("Invalid square name: " + name);

    return sq;
}

int legalCount(const string& fen)
{
    Board board;
    string error;

    if (!board.loadFEN(fen, &error))
    {
        cerr << "[ERROR] Invalid FEN: " << error << '\n';
        return -1;
    }

    return static_cast<int>(
        generateLegalMoves(board).size()
    );
}

GameState stateOf(const string& fen)
{
    Board board;
    string error;

    if (!board.loadFEN(fen, &error))
    {
        cerr << "[ERROR] Invalid FEN: " << error << '\n';
        return GameState::Playing;
    }

    return getGameState(board);
}

// ============================================================
// 1. FIND KING
// ============================================================

void testFindKing()
{
    cout << "\n--- Find King Tests ---\n";

    Board board;

    check(
        board.loadFEN("4k3/8/8/8/8/8/8/4K3 w - - 0 1"),
        "Load kings-only position"
    );

    check(
        sameSquare(findKing(board, Colour::white),
                   getSquare("e1")),
        "Find white king on e1"
    );

    check(
        sameSquare(findKing(board, Colour::black),
                   getSquare("e8")),
        "Find black king on e8"
    );

    check(
        board.loadFEN("8/8/8/8/8/8/8/8 w - - 0 1"),
        "Load position without kings"
    );

    Square missing = findKing(board, Colour::white);

    check(
        missing.r == -1 && missing.c == -1,
        "Missing king returns invalid square"
    );
}

// ============================================================
// 2. ATTACK DETECTION
// ============================================================

void testAttacks()
{
    cout << "\n--- Attack Detection Tests ---\n";

    Board board;

    // White rook attacks along a file.
    board.loadFEN("7k/8/8/8/8/8/4R3/K7 w - - 0 1");

    check(
        isSquareAttacked(board, getSquare("e7"), Colour::white),
        "Rook attacks along a file"
    );

    // A pawn blocks the rook.
    board.loadFEN("7k/8/8/8/4P3/8/4R3/K7 w - - 0 1");

    check(
        !isSquareAttacked(board, getSquare("e7"), Colour::white),
        "Own pawn blocks rook attack"
    );

    // White bishop on c1 attacks h6.
    board.loadFEN("7k/8/8/8/8/8/8/2B4K w - - 0 1");

    check(
        isSquareAttacked(board, getSquare("h6"), Colour::white),
        "Bishop attacks diagonally"
    );

    // White queen on d4 attacks d7.
    board.loadFEN("7k/8/8/8/3Q4/8/8/K7 w - - 0 1");

    check(
        isSquareAttacked(board, getSquare("d7"), Colour::white),
        "Queen attacks along a file"
    );

    // White knight on g1 attacks e2.
    board.loadFEN("7k/8/8/8/8/8/8/4K1N1 w - - 0 1");

    check(
        isSquareAttacked(board, getSquare("e2"), Colour::white),
        "Knight attacks in an L-shape"
    );

    // White king on e4 attacks d4.
    board.loadFEN("8/8/8/8/4K3/8/8/7k w - - 0 1");

    check(
        isSquareAttacked(board, getSquare("d4"), Colour::white),
        "King attacks adjacent square"
    );

    // White pawn on e4 attacks d5 and f5.
    board.loadFEN("7k/8/8/8/4P3/8/8/K7 w - - 0 1");

    check(
        isSquareAttacked(board, getSquare("d5"), Colour::white),
        "White pawn attacks diagonally left"
    );

    check(
        isSquareAttacked(board, getSquare("f5"), Colour::white),
        "White pawn attacks diagonally right"
    );

    // Black pawn on e5 attacks d4 and f4.
    board.loadFEN("7k/8/8/4p3/8/8/8/K7 w - - 0 1");

    check(
        isSquareAttacked(board, getSquare("d4"), Colour::black),
        "Black pawn attacks diagonally left"
    );

    check(
        isSquareAttacked(board, getSquare("f4"), Colour::black),
        "Black pawn attacks diagonally right"
    );

    // Black king on f5 attacks e4.
    board.loadFEN("8/8/8/5k2/4K3/8/8/8 w - - 0 1");

    check(
        isSquareAttacked(board, getSquare("e4"), Colour::black),
        "Black king attacks adjacent square"
    );
}

// ============================================================
// 3. CHECK DETECTION
// ============================================================

void testCheck()
{
    cout << "\n--- Check Tests ---\n";

    Board board;

    board.loadFEN("4r1k1/8/8/8/8/8/8/4K3 w - - 0 1");

    check(
        isInCheck(board, Colour::white),
        "White king is in check from rook"
    );

    board.loadFEN("4r1k1/8/8/8/4P3/8/8/4K3 w - - 0 1");

    check(
        !isInCheck(board, Colour::white),
        "Blocking piece prevents rook check"
    );

    board.loadFEN("4k3/8/8/8/8/8/4R3/4K3 b - - 0 1");

    check(
        isInCheck(board, Colour::black),
        "Black king is in check from rook"
    );

    board.loadFEN("4k3/8/8/8/8/8/8/4K3 w - - 0 1");

    check(
        !isInCheck(board, Colour::white),
        "King is not in check in quiet position"
    );
}

// ============================================================
// 4. PINNED PIECES
// ============================================================

void testPins()
{
    cout << "\n--- Pin Tests ---\n";

    const string pinnedKnight =
        "4r1k1/8/8/8/8/8/4N3/4K3 w - - 0 1";

    Board board;

    check(
        board.loadFEN(pinnedKnight),
        "Load pinned knight position"
    );

    vector<Move> moves = generateLegalMoves(board);

    int knightMoves = 0;

    for (const Move& move : moves)
    {
        if (sameSquare(move.from, getSquare("e2")))
            knightMoves++;
    }

    check(
        knightMoves == 0,
        "Pinned knight cannot move and expose its king"
    );

    check(
        legalCount(pinnedKnight) >= 1,
        "Other legal replies remain available"
    );

    // A king may capture an undefended checking rook.
    const string rookCapture =
        "4k3/8/8/8/8/8/4r3/4K3 w - - 0 1";

    board.loadFEN(rookCapture);
    moves = generateLegalMoves(board);

    bool canCaptureRook = false;

    for (const Move& move : moves)
    {
        if (sameSquare(move.from, getSquare("e1")) &&
            sameSquare(move.to, getSquare("e2")))
        {
            canCaptureRook = true;
            break;
        }
    }

    check(
        canCaptureRook,
        "King can capture undefended checking rook"
    );
}

// ============================================================
// 5. EN PASSANT EXPOSING THE KING
// ============================================================

void testEnPassantExposure()
{
    cout << "\n--- En Passant Exposure Tests ---\n";

    const string fen =
        "8/8/8/KPp4r/8/8/8/7k w - c6 0 1";

    Board board;

    check(
        board.loadFEN(fen),
        "Load en passant exposure position"
    );

    vector<Move> moves = generateLegalMoves(board);

    bool foundIllegalEnPassant = false;

    for (const Move& move : moves)
    {
        if (sameSquare(move.from, getSquare("b5")) &&
            sameSquare(move.to, getSquare("c6")) &&
            hasFlag(move, MOVE_EN_PASSANT))
        {
            foundIllegalEnPassant = true;
            break;
        }
    }

    check(
        !foundIllegalEnPassant,
        "En passant rejected when it exposes king to rook"
    );
}

// ============================================================
// 6. CASTLING LEGALITY
// ============================================================

void testCastlingLegality()
{
    cout << "\n--- Castling Legality Tests ---\n";

    const string fen =
        "5r1k/8/8/8/8/8/8/R3K2R w KQ - 0 1";

    Board board;

    check(
        board.loadFEN(fen),
        "Load castling position"
    );

    vector<Move> moves = generateLegalMoves(board);

    bool kingSideCastle = false;
    bool queenSideCastle = false;

    for (const Move& move : moves)
    {
        if (hasFlag(move, MOVE_KING_CASTLE))
            kingSideCastle = true;

        if (hasFlag(move, MOVE_QUEEN_CASTLE))
            queenSideCastle = true;
    }

    check(
        !kingSideCastle,
        "Kingside castling rejected through attacked square"
    );

    check(
        queenSideCastle,
        "Queenside castling remains legal"
    );

    // Verify the dedicated legality function as well.
    Move kingSide = makeMoveObject(
        getSquare("e1"), getSquare("g1"), MOVE_KING_CASTLE
    );

    Move queenSide = makeMoveObject(
        getSquare("e1"), getSquare("c1"), MOVE_QUEEN_CASTLE
    );

    check(
        !isCastlingLegal(board, kingSide),
        "isCastlingLegal rejects kingside castle"
    );

    check(
        isCastlingLegal(board, queenSide),
        "isCastlingLegal accepts queenside castle"
    );
}

// ============================================================
// 7. CHECKMATE AND STALEMATE
// ============================================================

void testCheckmateAndStalemate()
{
    cout << "\n--- Checkmate and Stalemate Tests ---\n";

    const string foolsMate =
        "rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3";

    const string stalemate =
        "7k/5Q2/6K1/8/8/8/8/8 b - - 0 1";

    check(
        legalCount(foolsMate) == 0,
        "Fool's mate has zero legal moves"
    );

    check(
        stateOf(foolsMate) == GameState::Checkmate,
        "Fool's mate detected as checkmate"
    );

    check(
        legalCount(stalemate) == 0,
        "Stalemate position has zero legal moves"
    );

    check(
        stateOf(stalemate) == GameState::Stalemate,
        "Stalemate detected correctly"
    );

    check(
        stateOf(
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
        ) == GameState::Playing,
        "Starting position is not terminal"
    );

    check(
        stateOf(
            "4k3/8/8/8/8/8/4r3/4K3 w - - 0 1"
        ) == GameState::Check,
        "Position with legal replies while checked reports Check"
    );
}

// ============================================================
// 8. INSUFFICIENT MATERIAL
// ============================================================


void testInsufficientMaterial()
{
    Board board;
    string error;

    auto test = [&](const string& name,
                    const string& fen,
                    bool expected)
    {
        bool loaded = board.loadFEN(fen, &error);

        if (!loaded)
        {
            cout << "[FAIL] " << name
                 << " - FEN error: " << error << '\n';
            return;
        }

        bool result = isInsufficientMaterial(board);

        if (result == expected)
            cout << "[PASS] " << name << '\n';
        else
            cout << "[FAIL] " << name
                 << " - expected " << expected
                 << ", got " << result << '\n';
    };

    cout << "\n--- Insufficient Material Tests ---\n";

    // 1. Kings only
    test("K vs K",
         "4k3/8/8/8/8/8/8/4K3 w - - 0 1",
         true);

    // 2. One bishop or knight against a bare king
    test("K+B vs K",
         "4k3/8/8/8/8/8/8/2B1K3 w - - 0 1",
         true);

    test("K+N vs K",
         "4k3/8/8/8/8/8/8/2N1K3 w - - 0 1",
         true);

    test("K vs K+B",
         "4k3/8/8/8/8/8/8/4K1b1 w - - 0 1",
         true);

    test("K vs K+N",
         "4k3/8/8/8/8/8/8/4K1n1 w - - 0 1",
         true);

    // 3. Bishop-only positions: all bishops on the same square colour
    test("K+2B vs K, bishops same colour",
     "4k3/8/8/8/8/8/2B5/4KB2 w - - 0 1",
     true);

    test("K+B vs K+B, bishops same colour",
         "4kb2/8/8/8/8/8/8/2B1K3 w - - 0 1",
         true);

    // 4. Bishop-only positions: bishops on opposite square colours
    test("K+B vs K+B, bishops opposite colours",
         "4k1b1/8/8/8/8/8/8/2B1K3 w - - 0 1",
         false);

    // 5. Knight combinations
    test("K+N vs K+N",
         "4k1n1/8/8/8/8/8/8/2N1K3 w - - 0 1",
         false);

    test("K+2N vs K",
         "4k3/8/8/8/8/8/2N5/1N2K3 w - - 0 1",
         false);

    // 6. Bishop + knight combinations
    test("K+B vs K+N",
         "4k1n1/8/8/8/8/8/8/2B1K3 w - - 0 1",
         false);

    test("K+N vs K+B",
         "4kb2/8/8/8/8/8/8/2N1K3 w - - 0 1",
         false);

    // 7. Pawns, rooks and queens
    test("K+P vs K",
         "4k3/8/8/8/8/8/P7/4K3 w - - 0 1",
         false);

    test("K+R vs K",
         "4k3/8/8/8/8/8/8/R3K3 w - - 0 1",
         false);

    test("K+Q vs K",
         "4k3/8/8/8/8/8/8/Q3K3 w - - 0 1",
         false);
}

// ============================================================
// 9. FIFTY-MOVE RULE
// ============================================================

void testFiftyMove()
{
    cout << "\n--- Fifty-Move Rule Tests ---\n";

    Board board;

    check(
        board.loadFEN("4k3/8/8/8/8/8/8/R3K3 w - - 99 50"),
        "Load halfmove clock 99"
    );

    check(
        !isFiftyMoveDraw(board),
        "Halfmove clock 99 is not a fifty-move draw"
    );

    check(
        board.loadFEN("4k3/8/8/8/8/8/8/R3K3 w - - 100 51"),
        "Load halfmove clock 100"
    );

    check(
        isFiftyMoveDraw(board),
        "Halfmove clock 100 triggers fifty-move draw"
    );
}

// ============================================================
// 10. THREEFOLD REPETITION AND UNDO
// ============================================================

void testRepetition()
{
    cout << "\n--- Threefold Repetition Tests ---\n";

    Board board;
    board.loadStartPosition();

    check(
        board.getPositionKeys().size() == 1,
        "Initial position is recorded"
    );

    auto playMove = [&](const string& from, const string& to)
    {
        board.makeMove(
            makeMoveObject(getSquare(from), getSquare(to))
        );
    };

    // First return to the starting position.
    playMove("g1", "f3");
    playMove("g8", "f6");
    playMove("f3", "g1");
    playMove("f6", "g8");

    check(
        repetitionCount(board) == 2,
        "Starting position has occurred twice"
    );

    // Second return to the starting position.
    playMove("g1", "f3");
    playMove("g8", "f6");
    playMove("f3", "g1");
    playMove("f6", "g8");

    check(
        repetitionCount(board) == 3,
        "Starting position has occurred three times"
    );

    check(
        isThreefoldRepetition(board),
        "Threefold repetition detected"
    );

    check(
        getGameState(board) == GameState::DrawRepetition,
        "Game state reports repetition draw"
    );

    check(
        board.getPositionKeys().size() == 9,
        "Nine position keys recorded after eight moves"
    );

    bool allUndone = true;

    for (int i = 0; i < 4; i++)
    {
        if (!board.undoMove())
        {
            allUndone = false;
            break;
        }
    }

    check(
        allUndone,
        "Undo four repetition-test moves"
    );

    check(
        board.getPositionKeys().size() == 5,
        "Undo removes position keys"
    );

    check(
        repetitionCount(board) == 2,
        "Undo reduces starting-position count to two"
    );

    check(
        !isThreefoldRepetition(board),
        "Repetition draw disappears after undo"
    );
}

// ============================================================
// MAIN
// ============================================================

int main()
{
    // testFindKing();
    // testAttacks();
    // testCheck();
    // testPins();
    // testEnPassantExposure();
    // testCastlingLegality();
    // testCheckmateAndStalemate();
    testInsufficientMaterial();
    // testFiftyMove();
    // testRepetition();

    // cout << "\n========== PERFT SUITE ==========\n";
    // runPerftSuite();

    cout << "\n========== TEST SUMMARY ==========\n";
    cout << "Passed: " << passed << '\n';
    cout << "Failed: " << failed << '\n';

    return failed == 0 ? 0 : 1;
}
