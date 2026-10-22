
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>

#include "../engine/evaluation.h"
#include "test_data.h"
#include "../engine/Board.h"
#include "../engine/rules.h"
#include "../engine/legal.h"
#include "../engine/MoveGenerator.h"
#include "../engine/attacks.h"
#include "test_framework.h"

using namespace std;
using TestFramework::check;
using TestFramework::checkEqual;

// ------------------------------------------------------------
// Shared helpers
// ------------------------------------------------------------

bool loadTestPosition(Board& board, const string& fen, const string& name)
{
    string error;

    bool loaded = board.loadFEN(fen, &error);

    check(
        name + ": FEN loads successfully",
        loaded,
        loaded ? "" : "FEN: " + fen + " | Error: " + error
    );

    return loaded;
}

// Load a FEN and return all legal moves.
vector<Move> legalMovesOf(const string& fen)
{
    Board board;

    if (!loadTestPosition(board, fen, "legalMovesOf")) {
        return {};
    }

    return generateLegalMoves(board);
}

// Return the number of legal moves in a position.
int moveCount(const string& fen)
{
    return static_cast<int>(legalMovesOf(fen).size());
}

// Check that a move written in coordinate notation exists.
void expectMove(
    const string& name,
    const vector<Move>& moves,
    const string& moveText
)
{
    bool found = false;

    for (const Move& move : moves) {
        if (moveToString(move) == moveText) {
            found = true;
            break;
        }
    }

    check(
        name,
        found,
        found ? "" : "Expected legal move not found: " + moveText
    );
}

// Check that a move written in coordinate notation does not exist.
void expectNoMove(
    const string& name,
    const vector<Move>& moves,
    const string& moveText
)
{
    bool found = false;

    for (const Move& move : moves) {
        if (moveToString(move) == moveText) {
            found = true;
            break;
        }
    }

    check(
        name,
        !found,
        found ? "Unexpected move found: " + moveText : ""
    );
}

// Count legal moves originating from a particular square.
void expectCountFrom(
    const string& name,
    const vector<Move>& moves,
    const string& squareName,
    int expectedCount
)
{
    Square square;
    bool validSquare = nameToSquare(squareName, square);

    if (!validSquare) {
        check(name + ": valid square name", false, squareName);
        return;
    }
    int actualCount = 0;

    for (const Move& move : moves) {
        if (move.from.r == square.r &&
            move.from.c == square.c) {
            ++actualCount;
        }
    }

    checkEqual(name, expectedCount, actualCount);
}

// Check the resulting game state for a FEN.
void expectState(const string& name,const string& fen,GameState expectedState)
{
    Board board;

    if (!loadTestPosition(board, fen, name)) {
        return;
    }

    checkEqual(
        name + ": game state",
        static_cast<int>(expectedState),
        static_cast<int>(getGameState(board))
    );
}

// Parse, validate, and play a sequence of legal moves.
bool playMoves(Board& board, const vector<string>& moveTexts)
{
    for (const string& moveText : moveTexts) {
        Move requestedMove;

        if (!stringToMove(moveText, requestedMove)) {
            check(
                "playMoves: move notation is valid: " + moveText,
                false
            );
            return false;
        }

        vector<Move> legalMoves = generateLegalMoves(board);
        bool found = false;
        Move selectedMove;

        for (const Move& candidate : legalMoves) {
            if (moveToString(candidate) == moveText) {
                selectedMove = candidate;
                found = true;
                break;
            }
        }

        if (!found) {
            check(
                "playMoves: move is legal: " + moveText,
                false
            );
            return false;
        }

        board.makeMove(selectedMove);
    }

    return true;
}


int countAllPieces(const Board& board)
{
    int count = 0;

    for (int r = 0; r < 8; ++r)
    {
        for (int c = 0; c < 8; ++c)
        {
            if (board.getPiece(r, c) != '.')
                ++count;
        }
    }

    return count;
}

// ------------------------------------------------------------
// Starting position tests
// ------------------------------------------------------------

void testStartPosition()
{
    const string startFEN =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    Board board;

    if (!loadTestPosition(board, startFEN, "Start position")) {
        return;
    }

    vector<Move> moves = generateLegalMoves(board);

    // The initial chess position has exactly 20 legal moves.
    checkEqual(
        "Start position: White has 20 legal moves",
        20,
        static_cast<int>(moves.size())
    );

    // All eight white pawns can move one square forward.
    const vector<string> singlePawnPushes = {
        "a2a3", "b2b3", "c2c3", "d2d3",
        "e2e3", "f2f3", "g2g3", "h2h3"
    };

    // All eight white pawns can move two squares forward.
    const vector<string> doublePawnPushes = {
        "a2a4", "b2b4", "c2c4", "d2d4",
        "e2e4", "f2f4", "g2g4", "h2h4"
    };

    // Each knight has two legal moves from the starting position.
    const vector<string> knightMoves = {
        "b1a3", "b1c3",
        "g1f3", "g1h3"
    };

    for (const string& move : singlePawnPushes) {
        expectMove(
            "Start position: single pawn push " + move,
            moves,
            move
        );
    }

    for (const string& move : doublePawnPushes) {
        expectMove(
            "Start position: double pawn push " + move,
            moves,
            move
        );
    }

    for (const string& move : knightMoves) {
        expectMove(
            "Start position: knight move " + move,
            moves,
            move
        );
    }

    // There should be no other moves from the starting position.
    checkEqual(
        "Start position: 16 pawn moves",
        16,
        static_cast<int>(
            count_if(
                moves.begin(),
                moves.end(),
                [](const Move& move) {
                    char piece = move.from.r == 7
                        ? '.'
                        : 'P';

                    // White's starting pawns are on row 6.
                    return move.from.r == 6 && piece == 'P';
                }
            )
        )
    );

    checkEqual(
        "Start position: 4 knight moves",
        4,
        static_cast<int>(
            count_if(
                moves.begin(),
                moves.end(),
                [](const Move& move) {
                    return move.from.r == 7 &&
                           (move.from.c == 1 ||
                            move.from.c == 6);
                }
            )
        )
    );

    // The legal move set must not contain moves from other pieces.
    for (const Move& move : moves) {
        bool fromPawn = move.from.r == 6;
        bool fromKnight =
            move.from.r == 7 &&
            (move.from.c == 1 || move.from.c == 6);

        check(
            "Start position: move originates from a pawn or knight",
            fromPawn || fromKnight,
            "Unexpected move: " + moveToString(move)
        );
    }

    // Make e2-e4, then verify that Black has 20 legal replies.
    Move e2e4;

    bool parsed = stringToMove("e2e4", e2e4);
    check("Start position: e2e4 notation parses", parsed);

    if (!parsed) {
        return;
    }

    bool foundE2E4 = false;

    for (const Move& move : moves) {
        if (moveToString(move) == "e2e4") {
            e2e4 = move;
            foundE2E4 = true;
            break;
        }
    }

    check("Start position: e2e4 is legal", foundE2E4);

    if (!foundE2E4) {
        return;
    }

    board.makeMove(e2e4);

    vector<Move> blackMoves = generateLegalMoves(board);

    checkEqual(
        "After 1.e4: Black has 20 legal replies",
        20,
        static_cast<int>(blackMoves.size())
    );
}


void testPawnMovement()
{
    // --------------------------------------------------------
    // 1. White pawn: single and double pushes
    // --------------------------------------------------------

    const string whitePawnStart =
        "4k3/8/8/8/8/8/4P3/4K3 w - - 0 1";

    vector<Move> moves = legalMovesOf(whitePawnStart);

    expectMove("White pawn: single push e2e3", moves, "e2e3");
    expectMove("White pawn: double push e2e4", moves, "e2e4");
    expectNoMove("White pawn: cannot move backward", moves, "e2e1");

    // --------------------------------------------------------
    // 2. Black pawn: single and double pushes
    // --------------------------------------------------------

    const string blackPawnStart =
        "4k3/4p3/8/8/8/8/8/4K3 b - - 0 1";

    moves = legalMovesOf(blackPawnStart);

    expectMove("Black pawn: single push e7e6", moves, "e7e6");
    expectMove("Black pawn: double push e7e5", moves, "e7e5");
    expectNoMove("Black pawn: cannot move backward", moves, "e7e8");

    // --------------------------------------------------------
    // 3. A pawn away from its starting rank cannot double push
    // --------------------------------------------------------

    const string whitePawnNotOnStart =
        "4k3/8/8/4P3/8/8/8/4K3 w - - 0 1";

    moves = legalMovesOf(whitePawnNotOnStart);

    expectMove("White pawn on e5: single push e5e6", moves, "e5e6");
    expectNoMove("White pawn on e5: no double push e5e7", moves, "e5e7");

    const string blackPawnNotOnStart =
        "4k3/8/4p3/8/8/8/8/4K3 b - - 0 1";

    moves = legalMovesOf(blackPawnNotOnStart);

    expectMove("Black pawn on e6: single push e6e5", moves, "e6e5");
    expectNoMove("Black pawn on e6: no double push e6e4", moves, "e6e4");

    // --------------------------------------------------------
    // 4. Blocked pawn pushes
    // --------------------------------------------------------

    // White pawn's immediate forward square is occupied.
    const string whitePawnBlockedOne =
        "4k3/8/8/8/4N3/4P3/8/4K3 w - - 0 1";

    moves = legalMovesOf(whitePawnBlockedOne);

    expectNoMove("White pawn: blocked single push", moves, "e3e4");
    expectNoMove("White pawn: blocked double push", moves, "e3e5");

    // Black pawn's immediate forward square is occupied.
    const string blackPawnBlockedOne =
        "4k3/4p3/4N3/8/8/8/8/4K3 b - - 0 1";

    moves = legalMovesOf(blackPawnBlockedOne);

    expectNoMove("Black pawn: blocked single push", moves, "e7e6");
    expectNoMove("Black pawn: blocked double push", moves, "e7e5");

    // The first square is empty, but the second square is blocked.
    const string whiteDoubleBlocked =
        "4k3/8/8/8/4N3/8/4P3/4K3 w - - 0 1";

    moves = legalMovesOf(whiteDoubleBlocked);

    expectMove("White pawn: single push when second square blocked", moves, "e2e3");
    expectNoMove("White pawn: double push blocked on e4", moves, "e2e4");

    const string blackDoubleBlocked =
        "4k3/4p3/8/4N3/8/8/8/4K3 b - - 0 1";

    moves = legalMovesOf(blackDoubleBlocked);

    expectMove("Black pawn: single push when second square blocked", moves, "e7e6");
    expectNoMove("Black pawn: double push blocked on e5", moves, "e7e5");

    // --------------------------------------------------------
    // 5. Pawn captures in both diagonal directions
    // --------------------------------------------------------

    // White pawn on e4 can capture black pieces on d5 and f5.
    const string whitePawnCaptures =
        "4k3/8/8/3n1n2/4P3/8/8/4K3 w - - 0 1";

    moves = legalMovesOf(whitePawnCaptures);

    expectMove("White pawn: capture left d5", moves, "e4d5");
    expectMove("White pawn: capture right f5", moves, "e4f5");

    // Black pawn on e5 can capture white pieces on d4 and f4.
    const string blackPawnCaptures =
        "4k3/8/8/4p3/3N1N2/8/8/4K3 b - - 0 1";

    moves = legalMovesOf(blackPawnCaptures);

    expectMove("Black pawn: capture left d4", moves, "e5d4");
    expectMove("Black pawn: capture right f4", moves, "e5f4");

    // --------------------------------------------------------
    // 6. Pawns cannot capture empty squares or friendly pieces
    // --------------------------------------------------------

    const string whiteNoDiagonalCapture =
        "4k3/8/8/8/4P3/8/8/4K3 w - - 0 1";

    moves = legalMovesOf(whiteNoDiagonalCapture);

    expectNoMove("White pawn: cannot capture empty d5", moves, "e4d5");
    expectNoMove("White pawn: cannot capture empty f5", moves, "e4f5");

    const string whiteFriendlyPiece =
        "4k3/8/8/3P1P2/4P3/8/8/4K3 w - - 0 1";

    moves = legalMovesOf(whiteFriendlyPiece);

    expectNoMove("White pawn: cannot capture friendly piece on d5", moves, "e4d5");
    expectNoMove("White pawn: cannot capture friendly piece on f5", moves, "e4f5");

    const string blackNoDiagonalCapture =
        "4k3/8/8/4p3/8/8/8/4K3 b - - 0 1";

    moves = legalMovesOf(blackNoDiagonalCapture);

    expectNoMove("Black pawn: cannot capture empty d4", moves, "e5d4");
    expectNoMove("Black pawn: cannot capture empty f4", moves, "e5f4");

    const string blackFriendlyPiece =
        "4k3/8/8/4p3/3p1p2/8/8/4K3 b - - 0 1";

    moves = legalMovesOf(blackFriendlyPiece);

    expectNoMove("Black pawn: cannot capture friendly piece on d4", moves, "e5d4");
    expectNoMove("Black pawn: cannot capture friendly piece on f4", moves, "e5f4");

    // --------------------------------------------------------
    // 7. Board edges: no wraparound captures
    // --------------------------------------------------------

    // White pawn on a-file must not wrap around to the h-file.
    const string whiteAFile =
        "4k3/8/8/8/8/8/P6p/4K3 w - - 0 1";

    moves = legalMovesOf(whiteAFile);

    expectMove("White a-pawn: normal push a2a3", moves, "a2a3");
    expectNoMove("White a-pawn: no wraparound capture", moves, "a2h3");

    // White pawn on h-file must not wrap around to the a-file.
    const string whiteHFile =
        "4k3/8/8/8/8/8/7P/4K3 w - - 0 1";

    moves = legalMovesOf(whiteHFile);

    expectMove("White h-pawn: normal push h2h3", moves, "h2h3");
    expectNoMove("White h-pawn: no wraparound capture", moves, "h2a3");

    // Black pawn on a-file must not wrap around to the h-file.
    const string blackAFile =
        "4k3/p6P/8/8/8/8/8/4K3 b - - 0 1";

    moves = legalMovesOf(blackAFile);

    expectMove("Black a-pawn: normal push a7a6", moves, "a7a6");
    expectNoMove("Black a-pawn: no wraparound capture", moves, "a7h6");

    // Black pawn on h-file must not wrap around to the a-file.
    const string blackHFile =
        "4k3/7p/8/8/8/8/8/4K3 b - - 0 1";

    moves = legalMovesOf(blackHFile);

    expectMove("Black h-pawn: normal push h7h6", moves, "h7h6");
    expectNoMove("Black h-pawn: no wraparound capture", moves, "h7a6");

    // --------------------------------------------------------
    // 8. Legal move generation must preserve the board
    // --------------------------------------------------------

    Board board;

    const string preservationFEN =
        "4k3/8/8/3n1n2/4P3/8/8/4K3 w - - 0 1";

    if (loadTestPosition(board, preservationFEN, "Pawn board preservation")) {
        const string beforeFEN = board.toFEN();
        const int beforeHistory = board.historySize();

        vector<Move> generatedMoves = generateLegalMoves(board);

        check(
            "Pawn generation: produces legal moves",
            !generatedMoves.empty()
        );

        checkEqual(
            "Pawn generation: board FEN unchanged",
            beforeFEN,
            board.toFEN()
        );

        checkEqual(
            "Pawn generation: undo history unchanged",
            beforeHistory,
            board.historySize()
        );
    }
}

void testKnightMovement()
{
    cout << "\n--- Knight Movement Tests ---\n";

    // 1. Knight in the center of an otherwise empty board.
    const string centerKnight =
        "4k3/8/8/8/3N4/8/8/4K3 w - - 0 1";

    vector<Move> moves = legalMovesOf(centerKnight);

    expectCountFrom(
        "White knight on d4 has 8 legal moves",
        moves,
        "d4",
        8
    );

    expectMove("Knight d4 to b3", moves, "d4b3");
    expectMove("Knight d4 to b5", moves, "d4b5");
    expectMove("Knight d4 to c2", moves, "d4c2");
    expectMove("Knight d4 to c6", moves, "d4c6");
    expectMove("Knight d4 to e2", moves, "d4e2");
    expectMove("Knight d4 to e6", moves, "d4e6");
    expectMove("Knight d4 to f3", moves, "d4f3");
    expectMove("Knight d4 to f5", moves, "d4f5");

    // 2. Knight in the corner: only two destinations are on the board.
    const string cornerKnight =
        "4k3/8/8/8/8/8/8/N3K3 w - - 0 1";

    moves = legalMovesOf(cornerKnight);

    expectCountFrom(
        "White knight on a1 has 2 legal moves",
        moves,
        "a1",
        2
    );

    expectMove("Knight a1 to b3", moves, "a1b3");
    expectMove("Knight a1 to c2", moves, "a1c2");

    // 3. Knight cannot capture a friendly piece.
    const string friendlyPiece =
        "4k3/8/8/8/3N4/8/2P5/4K3 w - - 0 1";

    moves = legalMovesOf(friendlyPiece);

    expectNoMove(
        "Knight cannot move to a friendly pawn",
        moves,
        "d4c2"
    );

    // 4. Knight can capture an enemy piece.
    const string enemyPiece =
        "4k3/8/8/8/3N4/2p5/8/4K3 w - - 0 1";

    moves = legalMovesOf(enemyPiece);

    expectMove(
        "Knight can capture an enemy pawn",
        moves,
        "d4c2"
    );

    // 5. Knight can jump over intervening pieces.
    const string jumpingKnight =
        "4k3/8/8/2P1P3/3N4/2P1P3/8/4K3 w - - 0 1";

    moves = legalMovesOf(jumpingKnight);

    expectMove("Knight jumps over pieces to b3", moves, "d4b3");
    expectMove("Knight jumps over pieces to b5", moves, "d4b5");
    expectMove("Knight jumps over pieces to f3", moves, "d4f3");
    expectMove("Knight jumps over pieces to f5", moves, "d4f5");

    // 6. Knight movement must not wrap around board edges.
    const string edgeKnight =
        "4k3/8/8/8/8/8/8/KN2K3 w - - 0 1";

    moves = legalMovesOf(edgeKnight);

    expectNoMove(
        "Knight on b1 cannot wrap around the board",
        moves,
        "b1h2"
    );

    // 7. Legal move generation must preserve the position.
    Board board;
    board.loadFEN(centerKnight);

    const string fenBefore = board.toFEN();
    const auto historyBefore = board.getPositionKeys();

    generateLegalMoves(board);

    checkEqual(
        "Knight generation leaves board FEN unchanged",
        fenBefore,
        board.toFEN()
    );

    check(
        "Knight generation leaves position history unchanged",
        historyBefore == board.getPositionKeys()
    );
}

void testBishopMovement()
{
    cout << "\n--- Bishop Movement Tests ---\n";

    // 1. Bishop in the center: all four diagonals.
    const string centerBishop =
        "4k3/8/8/8/3B4/8/8/4K3 w - - 0 1";

    vector<Move> moves = legalMovesOf(centerBishop);

    // From d4, the bishop has 13 moves on an otherwise empty board.
    expectCountFrom(
        "White bishop on d4 has 13 legal moves",
        moves,
        "d4",
        13
    );

    expectMove("Bishop d4 to a1", moves, "d4a1");
    expectMove("Bishop d4 to g1", moves, "d4g1");
    expectMove("Bishop d4 to a7", moves, "d4a7");
    expectMove("Bishop d4 to g7", moves, "d4g7");
    expectMove("Bishop d4 to e5", moves, "d4e5");
    expectMove("Bishop d4 to c5", moves, "d4c5");
    expectMove("Bishop d4 to e3", moves, "d4e3");
    expectMove("Bishop d4 to c3", moves, "d4c3");

    // 2. A friendly piece blocks the bishop.
    // White pawn on f6 blocks the diagonal d4-e5-f6-g7.
    const string friendlyBlock =
        "4k3/8/5P2/8/3B4/8/8/4K3 w - - 0 1";

    moves = legalMovesOf(friendlyBlock);

    expectMove("Bishop can move before friendly blocker", moves, "d4e5");

    expectNoMove(
        "Bishop cannot capture friendly pawn",
        moves,
        "d4f6"
    );

    expectNoMove(
        "Bishop cannot move beyond friendly pawn",
        moves,
        "d4g7"
    );

    // 3. An enemy piece can be captured, but the bishop cannot move beyond it.
    // Black pawn on f6 blocks the same diagonal.
    const string enemyBlock =
        "4k3/8/5p2/8/3B4/8/8/4K3 w - - 0 1";

    moves = legalMovesOf(enemyBlock);

    expectMove("Bishop can move before enemy blocker", moves, "d4e5");

    expectMove(
        "Bishop can capture enemy pawn",
        moves,
        "d4f6"
    );

    expectNoMove(
        "Bishop cannot move beyond captured piece",
        moves,
        "d4g7"
    );

    // 4. Bishop on a1: only the a1-h8 diagonal is available.
    const string cornerBishop =
        "4k3/8/8/8/8/8/8/B3K3 w - - 0 1";

    moves = legalMovesOf(cornerBishop);

    expectCountFrom(
        "White bishop on a1 has 7 legal moves",
        moves,
        "a1",
        7
    );

    expectMove("Bishop a1 to b2", moves, "a1b2");
    expectMove("Bishop a1 to h8", moves, "a1h8");

    expectNoMove(
        "Bishop cannot wrap around board edges",
        moves,
        "a1h2"
    );

    // 5. Legal move generation must preserve the position and history.
    Board board;
    board.loadFEN(centerBishop);

    const string fenBefore = board.toFEN();
    const auto historyBefore = board.getPositionKeys();

    generateLegalMoves(board);

    checkEqual(
        "Bishop generation leaves board FEN unchanged",
        fenBefore,
        board.toFEN()
    );

    check(
        "Bishop generation leaves position history unchanged",
        historyBefore == board.getPositionKeys()
    );
}

void testRookMovement()
{
    cout << "\n--- Rook Movement Tests ---\n";

    // 1. Rook in the center of an otherwise empty board.
    const string centerRook =
        "4k3/8/8/8/3R4/8/8/4K3 w - - 0 1";

    vector<Move> moves = legalMovesOf(centerRook);

    // A rook on d4 has 14 moves on an otherwise empty board.
    expectCountFrom(
        "White rook on d4 has 14 legal moves",
        moves,
        "d4",
        14
    );

    // Horizontal movement.
    expectMove("Rook d4 to a4", moves, "d4a4");
    expectMove("Rook d4 to b4", moves, "d4b4");
    expectMove("Rook d4 to h4", moves, "d4h4");

    // Vertical movement.
    expectMove("Rook d4 to d1", moves, "d4d1");
    expectMove("Rook d4 to d8", moves, "d4d8");

    // 2. Friendly pieces block the rook.
    // White pawn on d6 blocks the upward path.
    const string friendlyBlock =
        "4k3/8/3P4/8/3R4/8/8/4K3 w - - 0 1";

    moves = legalMovesOf(friendlyBlock);

    expectMove(
        "Rook can move before friendly blocker",
        moves,
        "d4d5"
    );

    expectNoMove(
        "Rook cannot capture friendly pawn",
        moves,
        "d4d6"
    );

    expectNoMove(
        "Rook cannot move beyond friendly pawn",
        moves,
        "d4d7"
    );

    // 3. An enemy piece can be captured, but the rook cannot move beyond it.
    // Black pawn on d6 blocks the upward path.
    const string enemyBlock =
        "4k3/8/3p4/8/3R4/8/8/4K3 w - - 0 1";

    moves = legalMovesOf(enemyBlock);

    expectMove(
        "Rook can move before enemy blocker",
        moves,
        "d4d5"
    );

    expectMove(
        "Rook can capture enemy pawn",
        moves,
        "d4d6"
    );

    expectNoMove(
        "Rook cannot move beyond captured piece",
        moves,
        "d4d7"
    );


    // 4. Rook in a corner.
    const string cornerRook =
    "4k3/8/8/8/8/8/8/R6K w - - 0 1";

    moves = legalMovesOf(cornerRook);

    // The rook has 7 vertical moves and 6 horizontal moves.
    expectCountFrom(
        "White rook on a1 has 13 legal moves",
        moves,
        "a1",
        13
    );

    expectMove("Rook a1 to a8", moves, "a1a8");
    expectMove("Rook a1 to d1", moves, "a1d1");
    expectMove("Rook a1 to g1", moves, "a1g1");

    expectNoMove(
        "Rook cannot capture its own king",
        moves,
        "a1h1"
    );

    expectNoMove(
        "Rook cannot move diagonally",
        moves,
        "a1h2"
    );

    // 5. Legal move generation must preserve the position and history.
    Board board;
    board.loadFEN(centerRook);

    const string fenBefore = board.toFEN();
    const auto historyBefore = board.getPositionKeys();

    generateLegalMoves(board);

    checkEqual(
        "Rook generation leaves board FEN unchanged",
        fenBefore,
        board.toFEN()
    );

    check(
        "Rook generation leaves position history unchanged",
        historyBefore == board.getPositionKeys()
    );
}

void testQueenMovement()
{
    cout << "\n--- Queen Movement Tests ---\n";

    // 1. Queen in the center of an otherwise empty board.
    const string centerQueen =
        "4k3/8/8/8/3Q4/8/8/4K3 w - - 0 1";

    vector<Move> moves = legalMovesOf(centerQueen);

    // A queen on d4 has 27 moves on an otherwise empty board.
    expectCountFrom(
        "White queen on d4 has 27 legal moves",
        moves,
        "d4",
        27
    );

    // Horizontal movement.
    expectMove("Queen d4 to a4", moves, "d4a4");
    expectMove("Queen d4 to h4", moves, "d4h4");

    // Vertical movement.
    expectMove("Queen d4 to d1", moves, "d4d1");
    expectMove("Queen d4 to d8", moves, "d4d8");

    // Diagonal movement.
    expectMove("Queen d4 to a1", moves, "d4a1");
    expectMove("Queen d4 to h8", moves, "d4h8");
    expectMove("Queen d4 to a7", moves, "d4a7");
    expectMove("Queen d4 to g1", moves, "d4g1");

    // 2. A friendly pawn blocks the queen.
    // The white pawn on f6 blocks the diagonal d4-e5-f6-g7.
    const string friendlyBlock =
        "4k3/8/5P2/8/3Q4/8/8/4K3 w - - 0 1";

    moves = legalMovesOf(friendlyBlock);

    expectMove(
        "Queen can move before friendly blocker",
        moves,
        "d4e5"
    );

    expectNoMove(
        "Queen cannot capture friendly pawn",
        moves,
        "d4f6"
    );

    expectNoMove(
        "Queen cannot move beyond friendly pawn",
        moves,
        "d4g7"
    );

    // 3. An enemy pawn can be captured, but the queen
    // cannot move beyond the pawn on the same diagonal.
    const string enemyBlock =
        "4k3/8/5p2/8/3Q4/8/8/4K3 w - - 0 1";

    moves = legalMovesOf(enemyBlock);

    expectMove(
        "Queen can move before enemy blocker",
        moves,
        "d4e5"
    );

    expectMove(
        "Queen can capture enemy pawn",
        moves,
        "d4f6"
    );

    expectNoMove(
        "Queen cannot move beyond captured piece",
        moves,
        "d4g7"
    );

    // 4. Queen in a corner with no pieces blocking its paths.
    // The white king is on h2, away from the queen's rank and file.
    const string cornerQueen =
        "4k3/8/8/8/8/8/7K/Q7 w - - 0 1";

    moves = legalMovesOf(cornerQueen);

    // From a1: 7 horizontal + 7 vertical + 7 diagonal = 21.
    expectCountFrom(
        "White queen on a1 has 21 legal moves",
        moves,
        "a1",
        21
    );

    expectMove("Queen a1 to a8", moves, "a1a8");
    expectMove("Queen a1 to h1", moves, "a1h1");
    expectMove("Queen a1 to h8", moves, "a1h8");
    expectMove("Queen a1 to d4", moves, "a1d4");

    expectNoMove(
        "Queen cannot move like a knight",
        moves,
        "a1h2"
    );

    // 5. Legal move generation must preserve the position and history.
    Board board;
    board.loadFEN(centerQueen);

    const string fenBefore = board.toFEN();
    const auto historyBefore = board.getPositionKeys();

    generateLegalMoves(board);

    checkEqual(
        "Queen generation leaves board FEN unchanged",
        fenBefore,
        board.toFEN()
    );

    check(
        "Queen generation leaves position history unchanged",
        historyBefore == board.getPositionKeys()
    );
}

void testKingMovement()
{
    cout << "\n--- King Movement Tests ---\n";

    // 1. King in the center.
    // The black king on e8 is too far away to attack these squares.
    const string centerKing =
        "4k3/8/8/8/3K4/8/8/8 w - - 0 1";

    vector<Move> moves = legalMovesOf(centerKing);

    expectCountFrom(
        "White king on d4 has 8 legal moves",
        moves,
        "d4",
        8
    );

    expectMove("King d4 to c3", moves, "d4c3");
    expectMove("King d4 to c4", moves, "d4c4");
    expectMove("King d4 to c5", moves, "d4c5");
    expectMove("King d4 to d3", moves, "d4d3");
    expectMove("King d4 to d5", moves, "d4d5");
    expectMove("King d4 to e3", moves, "d4e3");
    expectMove("King d4 to e4", moves, "d4e4");
    expectMove("King d4 to e5", moves, "d4e5");

    // 2. King in a corner.
    const string cornerKing =
        "4k3/8/8/8/8/8/8/K7 w - - 0 1";

    moves = legalMovesOf(cornerKing);

    expectCountFrom(
        "White king on a1 has 3 legal moves",
        moves,
        "a1",
        3
    );

    expectMove("King a1 to a2", moves, "a1a2");
    expectMove("King a1 to b1", moves, "a1b1");
    expectMove("King a1 to b2", moves, "a1b2");

    // 3. King cannot capture a friendly piece.
    // White king is on d4; white pawn is on e4.
    const string friendlyPiece =
        "4k3/8/8/8/3KP3/8/8/8 w - - 0 1";

    moves = legalMovesOf(friendlyPiece);

    expectNoMove(
        "King cannot capture a friendly pawn",
        moves,
        "d4e4"
    );

    // 4. King can capture an undefended enemy pawn.
    // Black pawn on e4 is not protected by the black king on e8.
    const string enemyPiece =
        "4k3/8/8/8/3Kp3/8/8/8 w - - 0 1";

    moves = legalMovesOf(enemyPiece);

    expectMove(
        "King can capture a safe enemy pawn",
        moves,
        "d4e4"
    );

    // 5. King cannot move onto a square attacked by an enemy rook.
    // Black rook on e8 attacks the e-file, including e4.
    const string attackedSquare =
        "k3r3/8/8/8/3K4/8/8/8 w - - 0 1";

    moves = legalMovesOf(attackedSquare);

    expectNoMove(
        "King cannot move into rook attack",
        moves,
        "d4e4"
    );

    // 6. King cannot move onto a square controlled by the enemy king.
    // White king on d4; black king on f5.
    // The square e5 is adjacent to the black king and therefore attacked.
    const string enemyKingNearby =
        "8/8/8/5k2/3K4/8/8/8 w - - 0 1";

    moves = legalMovesOf(enemyKingNearby);

    expectNoMove(
        "King cannot move onto a square controlled by enemy king",
        moves,
        "d4e5"
    );

    // 7. Legal move generation must preserve the position and history.
    Board board;
    board.loadFEN(centerKing);

    const string fenBefore = board.toFEN();
    const auto historyBefore = board.getPositionKeys();

    generateLegalMoves(board);

    checkEqual(
        "King generation leaves board FEN unchanged",
        fenBefore,
        board.toFEN()
    );

    check(
        "King generation leaves position history unchanged",
        historyBefore == board.getPositionKeys()
    );
}


void testPins()
{
    cout << "\n--- Pin Tests ---\n";

    // 1. Pinned knight.
    // Black rook on e8 attacks down the e-file.
    // The white knight on e2 blocks the attack on the white king e1.
    // Any knight move exposes the king to the rook.
    const string pinnedKnight =
        "k3r3/8/8/8/8/8/4N3/4K3 w - - 0 1";

    vector<Move> moves = legalMovesOf(pinnedKnight);

    expectCountFrom(
        "Pinned knight on e2 has no legal moves",
        moves,
        "e2",
        0
    );

    expectNoMove(
        "Pinned knight cannot move to c3",
        moves,
        "e2c3"
    );

    expectNoMove(
        "Pinned knight cannot move to g3",
        moves,
        "e2g3"
    );

    // 2. Pinned rook.
    // The rook may move along the e-file while continuing to block
    // the black rook's attack on the white king.
    const string pinnedRook =
        "k3r3/8/8/8/8/8/4R3/4K3 w - - 0 1";

    moves = legalMovesOf(pinnedRook);

    expectMove(
        "Pinned rook can move along the pinning file",
        moves,
        "e2e3"
    );

    expectMove(
        "Pinned rook can move farther along the pinning file",
        moves,
        "e2e7"
    );

    expectMove(
        "Pinned rook can capture the pinning rook",
        moves,
        "e2e8"
    );

    expectNoMove(
        "Pinned rook cannot move sideways",
        moves,
        "e2d2"
    );

    expectNoMove(
        "Pinned rook cannot move to f2",
        moves,
        "e2f2"
    );

    // 3. Pinned pawn.
    // The white pawn on e2 blocks the black rook's attack on the
    // white king e1. Black pawns on d3 and f3 give the white pawn
    // potential capture targets, but capturing either exposes the king.
    const string pinnedPawn =
        "k3r3/8/8/8/8/3p1p2/4P3/4K3 w - - 0 1";

    moves = legalMovesOf(pinnedPawn);

    expectMove(
        "Pinned pawn can advance one square along the file",
        moves,
        "e2e3"
    );

    expectMove(
        "Pinned pawn can advance two squares along the file",
        moves,
        "e2e4"
    );

    expectNoMove(
        "Pinned pawn cannot capture to the left",
        moves,
        "e2d3"
    );

    expectNoMove(
        "Pinned pawn cannot capture to the right",
        moves,
        "e2f3"
    );

    // 4. Legal move generation must preserve the board and history.
    Board board;
    board.loadFEN(pinnedRook);

    const string fenBefore = board.toFEN();
    const auto historyBefore = board.getPositionKeys();

    generateLegalMoves(board);

    checkEqual(
        "Pin tests leave board FEN unchanged",
        fenBefore,
        board.toFEN()
    );

    check(
        "Pin tests leave position history unchanged",
        historyBefore == board.getPositionKeys()
    );
}


void testCheck()
{
    cout << "\n--- Check Tests ---\n";

    // Test 1: Black king is currently in check from a white rook.
    const string blackInCheck =
        "4k3/8/8/8/8/8/8/K3R3 b - - 0 1";

    Board board;
    board.loadFEN(blackInCheck);

    check("Black king in check is detected",
          getGameState(board) == GameState::Check);

    vector<Move> moves = legalMovesOf(blackInCheck);

    expectMove("Black king can escape check by moving to d8",
               moves, "e8d8");

    expectNoMove("Black king cannot remain on the attacked e-file",
                 moves, "e8e7");

    // Test 2: A white rook move gives check to the black king.
    const string beforeGivingCheck =
        "k7/8/8/8/8/8/8/4R2K w - - 0 1";

    Board checkingBoard;
    checkingBoard.loadFEN(beforeGivingCheck);

    moves = legalMovesOf(beforeGivingCheck);

    expectMove("White rook can move to e8 and give check",
               moves, "e1e8");

    Move checkingMove;
    bool found = false;

    for (const Move& move : moves)
    {
        if (moveToString(move) == "e1e8")
        {
            checkingMove = move;
            found = true;
            break;
        }
    }

    check("Checking rook move is generated", found);

    if (found)
    {
        checkingBoard.makeMove(checkingMove);

        check("Black king is detected in check after the rook move",
              getGameState(checkingBoard) == GameState::Check);
    }
}

// ------------------------------------------------------------
// Existing game-rule tests
// ------------------------------------------------------------


void testCheckmate()
{
    cout << "\n--- Checkmate Tests ---\n";

    Board board;

    const string checkmateFEN =
        "7k/6Q1/5K2/8/8/8/8/8 b - - 0 1";

    if (loadTestPosition(board, checkmateFEN, "Checkmate position"))
    {
        check("Position is checkmate", isCheckmate(board));
        check("Checkmate is not stalemate", !isStalemate(board));

        checkEqual(
            "Checkmate game state",
            static_cast<int>(GameState::Checkmate),
            static_cast<int>(getGameState(board))
        );
    }
}

void testStalemate()
{
    cout << "\n--- Stalemate Tests ---\n";

    Board board;

    const string stalemateFEN =
        "7k/5Q2/6K1/8/8/8/8/8 b - - 0 1";

    if (loadTestPosition(board, stalemateFEN, "Stalemate position"))
    {
        check("Position is stalemate", isStalemate(board));
        check("Stalemate is not checkmate", !isCheckmate(board));

        checkEqual(
            "Stalemate game state",
            static_cast<int>(GameState::Stalemate),
            static_cast<int>(getGameState(board))
        );
    }
}


void testDrawRules()
{
    cout << "\n--- Draw Rules Tests ---\n";

    Board board;

    // 1. Insufficient material
    const string kingsOnly =
        "4k3/8/8/8/8/8/8/4K3 w - - 0 1";

    if (loadTestPosition(board, kingsOnly, "Kings only")) {
        check(
            "Kings only is insufficient material",
            isInsufficientMaterial(board)
        );

        checkEqual(
            "Kings only game state",
            static_cast<int>(GameState::DrawInsufficient),
            static_cast<int>(getGameState(board))
        );
    }

    const string kingKnight =
        "4k3/8/8/8/8/8/8/3NK3 w - - 0 1";

    if (loadTestPosition(board, kingKnight, "King and knight")) {
        check(
            "King and knight versus king is insufficient",
            isInsufficientMaterial(board)
        );
    }

    const string kingBishop =
        "4k3/8/8/8/8/8/8/2B1K3 w - - 0 1";

    if (loadTestPosition(board, kingBishop, "King and bishop")) {
        check(
            "King and bishop versus king is insufficient",
            isInsufficientMaterial(board)
        );
    }

    const string extraMaterial =
        "4k3/8/8/8/8/8/8/2B1K2R w - - 0 1";

    if (loadTestPosition(board, extraMaterial, "Extra material")) {
        check(
            "Rook material is not insufficient",
            !isInsufficientMaterial(board)
        );
    }

    // 2. Fifty-move rule
    const string halfmove99 =
        "4k3/8/8/8/8/8/8/R3K3 w - - 99 1";

    if (loadTestPosition(board, halfmove99, "Halfmove clock 99")) {
        check(
            "99 halfmoves is not a fifty-move draw",
            !isFiftyMoveDraw(board)
        );
    }

    const string halfmove100 =
        "4k3/8/8/8/8/8/8/R3K3 w - - 100 1";

    if (loadTestPosition(board, halfmove100, "Halfmove clock 100")) {
        check(
            "100 halfmoves is a fifty-move draw",
            isFiftyMoveDraw(board)
        );

        checkEqual(
            "Fifty-move draw game state",
            static_cast<int>(GameState::DrawFiftyMove),
            static_cast<int>(getGameState(board))
        );
    }

    const string halfmove150 =
        "4k3/8/8/8/8/8/8/R3K3 w - - 150 1";

    if (loadTestPosition(board, halfmove150, "Halfmove clock 150")) {
        check(
            "150 halfmoves is a fifty-move draw",
            isFiftyMoveDraw(board)
        );
    }

    // 3. Threefold repetition
    const string startFEN =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    if (!loadTestPosition(board, startFEN, "Repetition start position")) {
        return;
    }

    checkEqual(
        "Initial position repetition count",
        1,
        repetitionCount(board)
    );

    check(
        "Initial position is not threefold repetition",
        !isThreefoldRepetition(board)
    );

    const vector<string> cycle = {
        "g1f3", "g8f6", "f3g1", "f6g8",
        "g1f3", "g8f6", "f3g1", "f6g8"
    };

    if (!playMoves(board, cycle)) {
        return;
    }

    checkEqual(
        "Repeated starting position count",
        3,
        repetitionCount(board)
    );

    check(
        "Position is threefold repetition",
        isThreefoldRepetition(board)
    );

    checkEqual(
        "Threefold repetition game state",
        static_cast<int>(GameState::DrawRepetition),
        static_cast<int>(getGameState(board))
    );
}

void testGameStateStrings()
{
    checkEqual(
        "Playing state string",
        string("Playing"),
        gameStateToString(GameState::Playing)
    );

    checkEqual(
        "Check state string",
        string("Check"),
        gameStateToString(GameState::Check)
    );

    checkEqual(
        "Checkmate state string",
        string("Checkmate"),
        gameStateToString(GameState::Checkmate)
    );

    checkEqual(
        "Stalemate state string",
        string("Stalemate"),
        gameStateToString(GameState::Stalemate)
    );

    checkEqual(
        "Insufficient material state string",
        string("Draw by insufficient material"),
        gameStateToString(GameState::DrawInsufficient)
    );

    checkEqual(
        "Fifty-move state string",
        string("Draw by fifty-move rule"),
        gameStateToString(GameState::DrawFiftyMove)
    );

    checkEqual(
        "Repetition state string",
        string("Draw by repetition"),
        gameStateToString(GameState::DrawRepetition)
    );
}


void testCastling()
{
    cout << "\n--- Castling Tests ---\n";

    // 1. All four castling moves.
    const string castlePosition =
        "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1";

    vector<Move> moves = legalMovesOf(castlePosition);

    expectMove("White kingside castling", moves, "e1g1");
    expectMove("White queenside castling", moves, "e1c1");

    const string blackCastlePosition =
        "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1";

    moves = legalMovesOf(blackCastlePosition);

    expectMove("Black kingside castling", moves, "e8g8");
    expectMove("Black queenside castling", moves, "e8c8");

    // 2. Castling is blocked by a friendly piece on f1.
    const string blockedCastle =
        "4k3/8/8/8/8/8/8/R3KB1R w KQ - 0 1";

    moves = legalMovesOf(blockedCastle);

    expectNoMove(
        "Kingside castling blocked by bishop on f1",
        moves,
        "e1g1"
    );

    expectMove(
        "Queenside castling remains available when kingside is blocked",
        moves,
        "e1c1"
    );

    // 3. A king in check cannot castle.
    // The black rook on e8 attacks the white king on e1.
    const string castleWhileInCheck =
        "k3r3/8/8/8/8/8/8/R3K2R w KQ - 0 1";

    moves = legalMovesOf(castleWhileInCheck);

    expectNoMove(
        "Cannot castle while in check: kingside",
        moves,
        "e1g1"
    );

    expectNoMove(
        "Cannot castle while in check: queenside",
        moves,
        "e1c1"
    );

    // 4. The king cannot castle through an attacked square.
    // The black rook on f8 attacks f1, the kingside transit square.
    const string castleThroughCheck =
        "k4r2/8/8/8/8/8/8/R3K2R w KQ - 0 1";

    moves = legalMovesOf(castleThroughCheck);

    expectNoMove(
        "Cannot castle through an attacked square",
        moves,
        "e1g1"
    );

    // 5. The king cannot castle onto an attacked square.
    // The black rook on g8 attacks g1, the kingside destination.
    const string castleLandingOnCheck =
        "k5r1/8/8/8/8/8/8/R3K2R w KQ - 0 1";

    moves = legalMovesOf(castleLandingOnCheck);

    expectNoMove(
        "Cannot castle onto an attacked square",
        moves,
        "e1g1"
    );

    // 6. An attacked b1 square does not prevent queenside castling.
    // The king travels through d1 to c1. It does not pass through b1.
    const string queenSideBFileAttacked =
        "1r2k3/8/8/8/8/8/8/R3K2R w KQ - 0 1";

    moves = legalMovesOf(queenSideBFileAttacked);

    expectMove(
        "Queenside castling allowed when only b1 is attacked",
        moves,
        "e1c1"
    );

    // 7. Moving the king removes both castling rights.
    // Undoing the move must restore both rights and the original FEN.
    Board board;

    const string rightsFen =
        "4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1";

    board.loadFEN(rightsFen);

    Move kingMove;
    const bool parsed = stringToMove("e1e2", kingMove);

    check("King move for castling-right test parses", parsed);

    if (parsed)
    {
        board.makeMove(kingMove);

        Castling rightsAfterMove = board.getCastlingRights();

        check(
            "King move removes white kingside castling right",
            !rightsAfterMove.wk
        );

        check(
            "King move removes white queenside castling right",
            !rightsAfterMove.wq
        );

        board.undoMove();

        checkEqual(
            "Undo restores original castling-test FEN",
            rightsFen,
            board.toFEN()
        );

        Castling rightsAfterUndo = board.getCastlingRights();

        check(
            "Undo restores white kingside castling right",
            rightsAfterUndo.wk
        );

        check(
            "Undo restores white queenside castling right",
            rightsAfterUndo.wq
        );
    }
}

void testEnPassant()
{
    cout << "\n--- En Passant Tests ---\n";

    // Helper: find an actual generated move so its special flags are preserved.
    auto findMove = [](const vector<Move>& moves,
                       const string& notation,
                       Move& result) -> bool
    {
        for (const Move& move : moves)
        {
            if (moveToString(move) == notation)
            {
                result = move;
                return true;
            }
        }

        return false;
    };

    // 1. En passant becomes available immediately after e2-e4.
    // Black pawn on d4 can capture the white pawn on e4 via e3.
    const string beforeDoublePush =
        "4k3/8/8/8/3p4/8/4P3/4K3 w - - 0 1";

    Board board;
    board.loadFEN(beforeDoublePush);

    vector<Move> moves = legalMovesOf(beforeDoublePush);

    Move doublePush;
    const bool foundDoublePush = findMove(moves, "e2e4", doublePush);

    check("White double pawn push e2e4 is generated", foundDoublePush);

    if (foundDoublePush)
    {
        check(
            "e2e4 carries the double-push flag",
            hasFlag(doublePush, MOVE_DOUBLE_PUSH)
        );

        board.makeMove(doublePush);

        moves = legalMovesOf(board.toFEN());

        expectMove(
            "En passant available immediately after e2e4",
            moves,
            "d4e3"
        );
    }

    // 2. En passant expires if Black makes another move.
    // After e2-e4, Black plays Ke8-e7; White then plays Ke1-e2.
    // Black must no longer have the d4xe3 en-passant capture.
    Board expiryBoard;
    expiryBoard.loadFEN(beforeDoublePush);

    moves = legalMovesOf(beforeDoublePush);

    Move move;
    bool found = findMove(moves, "e2e4", move);

    check("Expiry test: e2e4 is generated", found);

    if (found)
    {
        expiryBoard.makeMove(move);

        moves = legalMovesOf(expiryBoard.toFEN());

        found = findMove(moves, "e8e7", move);
        check("Expiry test: black king move e8e7 is generated", found);

        if (found)
        {
            expiryBoard.makeMove(move);

            moves = legalMovesOf(expiryBoard.toFEN());

            found = findMove(moves, "e1e2", move);
            check("Expiry test: white king move e1e2 is generated", found);

            if (found)
            {
                expiryBoard.makeMove(move);

                moves = legalMovesOf(expiryBoard.toFEN());

                expectNoMove(
                    "En passant expires after Black declines it",
                    moves,
                    "d4e3"
                );
            }
        }
    }

    // 3. En passant removes the correct pawn.
    // White pawn on e5 captures the black pawn on d5 by moving to d6.
    const string enPassantCapture =
        "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1";

    Board captureBoard;
    captureBoard.loadFEN(enPassantCapture);

    moves = legalMovesOf(enPassantCapture);

    expectMove(
        "White pawn can capture en passant e5d6",
        moves,
        "e5d6"
    );

    Move epMove;
    const bool foundEpMove = findMove(moves, "e5d6", epMove);

    check("En-passant capture move is generated", foundEpMove);

    if (foundEpMove)
    {
        check(
            "En-passant move has the en-passant flag",
            hasFlag(epMove, MOVE_EN_PASSANT)
        );

        captureBoard.makeMove(epMove);

        check(
            "White pawn lands on d6",
            captureBoard.getPiece(2, 3) == 'P'
        );

        check(
            "Captured black pawn is removed from d5",
            captureBoard.getPiece(3, 3) == EMPTY
        );

        check(
            "White pawn's original square e5 is empty",
            captureBoard.getPiece(3, 4) == EMPTY
        );
    }

    // 4. En passant is illegal if it exposes the moving side's king.
    // Black rook on a5 is blocked from the white king on h5 by the
    // white pawn on f5 and black pawn on g5. After f5xg6 en passant,
    // both pawns leave rank 5, exposing the white king to the rook.
    const string enPassantExposesKing =
        "4k3/8/8/r4PpK/8/8/8/8 w - g6 0 1";

    moves = legalMovesOf(enPassantExposesKing);

    expectNoMove(
        "En passant rejected when it exposes the king",
        moves,
        "f5g6"
    );
}

void testPromotion()
{
    cout << "\n--- Promotion Tests ---\n";

    // Helper: find an actual generated move so its flags are preserved.
    auto findMove = [](const vector<Move>& moves,
                       const string& notation,
                       Move& result) -> bool
    {
        for (const Move& move : moves)
        {
            if (moveToString(move) == notation)
            {
                result = move;
                return true;
            }
        }

        return false;
    };

    // 1. Four quiet promotion choices.
    const string quietPromotion =
        "4k3/P7/8/8/8/8/8/4K3 w - - 0 1";

    vector<Move> moves = legalMovesOf(quietPromotion);

    expectCountFrom(
        "White pawn on a7 has 4 promotion moves",
        moves,
        "a7",
        4
    );

    expectMove("Promote a7a8 to queen", moves, "a7a8q");
    expectMove("Promote a7a8 to rook", moves, "a7a8r");
    expectMove("Promote a7a8 to bishop", moves, "a7a8b");
    expectMove("Promote a7a8 to knight", moves, "a7a8n");

    // 2. Four quiet promotions plus four capture promotions.
    // Black rook on b8 can be captured by the white pawn on a7.
    const string capturePromotion =
        "1r2k3/P7/8/8/8/8/8/4K3 w - - 0 1";

    moves = legalMovesOf(capturePromotion);

    expectCountFrom(
        "White pawn on a7 has 8 promotions including captures",
        moves,
        "a7",
        8
    );

    expectMove("Capture-promote to queen", moves, "a7b8q");
    expectMove("Capture-promote to rook", moves, "a7b8r");
    expectMove("Capture-promote to bishop", moves, "a7b8b");
    expectMove("Capture-promote to knight", moves, "a7b8n");

    // 3. White promotion produces a white piece and undo restores the pawn.
    Board whiteBoard;
    whiteBoard.loadFEN(quietPromotion);

    const string originalWhiteFen = whiteBoard.toFEN();

    moves = legalMovesOf(originalWhiteFen);

    Move whitePromotion;
    bool found = findMove(moves, "a7a8q", whitePromotion);

    check("White queen-promotion move is generated", found);

    if (found)
    {
        check(
            "White promotion has the promotion flag",
            hasFlag(whitePromotion, MOVE_PROMOTION)
        );

        whiteBoard.makeMove(whitePromotion);

        check(
            "White promotion creates a white queen",
            whiteBoard.getPiece(0, 0) == 'Q'
        );

        whiteBoard.undoMove();

        checkEqual(
            "Undo promotion restores the original FEN",
            originalWhiteFen,
            whiteBoard.toFEN()
        );

        check(
            "Undo promotion restores the white pawn on a7",
            whiteBoard.getPiece(1, 0) == 'P'
        );
    }

    // 4. Black promotion produces a black piece.
    const string blackPromotionFen =
        "4k3/8/8/8/8/8/p7/4K3 b - - 0 1";

    Board blackBoard;
    blackBoard.loadFEN(blackPromotionFen);

    moves = legalMovesOf(blackPromotionFen);

    expectCountFrom(
        "Black pawn on a2 has 4 promotion moves",
        moves,
        "a2",
        4
    );

    expectMove("Black promotes to queen", moves, "a2a1q");
    expectMove("Black promotes to rook", moves, "a2a1r");
    expectMove("Black promotes to bishop", moves, "a2a1b");
    expectMove("Black promotes to knight", moves, "a2a1n");

    Move blackPromotion;
    found = findMove(moves, "a2a1q", blackPromotion);

    check("Black queen-promotion move is generated", found);

    if (found)
    {
        blackBoard.makeMove(blackPromotion);

        check(
            "Black promotion creates a black queen",
            blackBoard.getPiece(7, 0) == 'q'
        );
    }

    // 5. Promotion can give check.
    // A white queen on g8 attacks the black king on e8 along rank 8.
    const string promotionCheckFen =
        "4k3/6P1/8/8/8/8/8/4K3 w - - 0 1";

    Board checkBoard;
    checkBoard.loadFEN(promotionCheckFen);

    moves = legalMovesOf(promotionCheckFen);

    expectMove(
        "White pawn can promote on g8",
        moves,
        "g7g8q"
    );

    Move checkingPromotion;
    found = findMove(moves, "g7g8q", checkingPromotion);

    check("Checking promotion move is generated", found);

    if (found)
    {
        checkBoard.makeMove(checkingPromotion);

        check(
            "Promotion gives check to the black king",
            getGameState(checkBoard) == GameState::Check
        );
    }

    // 6. A pinned pawn cannot capture-promote if doing so exposes its king.
    // White pawn e7 blocks the black rook on e8 from the white king on e1.
    // Capturing the black bishop on d8 would move the pawn off the e-file
    // and expose the white king to the rook.
    const string pinnedPromotionFen =
        "k2br3/4P3/8/8/8/8/8/4K3 w - - 0 1";

    moves = legalMovesOf(pinnedPromotionFen);

    expectNoMove(
        "Pinned pawn cannot capture-promote to queen",
        moves,
        "e7d8q"
    );

    expectNoMove(
        "Pinned pawn cannot capture-promote to rook",
        moves,
        "e7d8r"
    );

    expectNoMove(
        "Pinned pawn cannot capture-promote to bishop",
        moves,
        "e7d8b"
    );

    expectNoMove(
        "Pinned pawn cannot capture-promote to knight",
        moves,
        "e7d8n"
    );
}


void testRegressions()
{
    cout << "\n--- Regression Tests ---\n";

    // Regression 1: A pinned rook cannot move sideways.
    const string pinnedRookFEN =
        "4r1k1/8/8/8/8/8/4R3/4K3 w - - 0 1";

    vector<Move> moves = legalMovesOf(pinnedRookFEN);

    expectMove(
        "Regression: pinned rook can move along the e-file",
        moves, "e2e3"
    );

    expectNoMove(
        "Regression: pinned rook cannot move sideways",
        moves, "e2d2"
    );

    // Regression 2: En passant cannot expose the king to a rook.
    const string enPassantPinFEN =
        "4k3/8/8/r4PpK/8/8/8/8 w - g6 0 1";

    moves = legalMovesOf(enPassantPinFEN);

    expectNoMove(
        "Regression: en passant cannot expose the king",
        moves, "f5g6"
    );

    // Regression 3: Castling cannot cross an attacked square.
    const string castleThroughCheckFEN =
        "k4r2/8/8/8/8/8/8/R3K2R w KQ - 0 1";

    moves = legalMovesOf(castleThroughCheckFEN);

    expectNoMove(
        "Regression: kingside castling through check is illegal",
        moves, "e1g1"
    );
}

// ------------------------------------------------------------
// Test-suite entry point
// ------------------------------------------------------------


void testMakeUndoIdentity(const string& fen)
{
    Board original;

    if (!loadTestPosition(
            original,
            fen,
            "Make/undo identity: load position"))
    {
        return;
    }

    const vector<Move> moves = generateLegalMoves(original);

    check(
        "Make/undo identity: position has legal moves",
        !moves.empty(),
        "No legal moves available in this position."
    );

    for (const Move& move : moves)
    {
        Board board = original;

        board.makeMove(move);
        const bool undoSucceeded = board.undoMove();

        const string moveName = moveToString(move);

        check(
            "Make/undo identity: undo succeeds for " + moveName,
            undoSucceeded
        );

        check(
            "Make/undo identity: state restored after " + moveName,
            board.isSameAs(original),
            "Board state differs after making and undoing " + moveName
        );
    }
}


void testDeepUndo(unsigned int seed)
{
    const string startFen =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    Board original;

    if (!loadTestPosition(
            original,
            startFen,
            "Deep undo: load starting position"))
    {
        return;
    }

    Board board = original;

    // Random number generator with a reproducible seed.
    mt19937 rng(seed);

    // Store the exact moves played so we can undo the entire sequence.
    vector<Move> playedMoves;

    const int targetPlies = 200;

    for (int ply = 0; ply < targetPlies; ++ply)
    {
        vector<Move> legalMoves = generateLegalMoves(board);

        // Stop if the position is checkmate or stalemate.
        if (legalMoves.empty())
            break;

        uniform_int_distribution<size_t> distribution(
            0,
            legalMoves.size() - 1
        );

        const Move selectedMove = legalMoves[distribution(rng)];

        board.makeMove(selectedMove);
        playedMoves.push_back(selectedMove);
    }

    check(
        "Deep undo: played at least 100 plies",
        playedMoves.size() >= 100,
        "Actual plies played: " + to_string(playedMoves.size())
    );

    // Undo every move in reverse order.
    bool allUndosSucceeded = true;

    for (size_t i = 0; i < playedMoves.size(); ++i)
    {
        if (!board.undoMove())
        {
            allUndosSucceeded = false;
            break;
        }
    }

    check(
        "Deep undo: every undo succeeded",
        allUndosSucceeded
    );

    check(
        "Deep undo: original state restored",
        board.isSameAs(original),
        "Board state, clocks, castling rights, en passant, "
        "repetition history, or undo-history size differs."
    );
}



void testMakeUndoIdentityAll()
{
    std::vector<std::string> testedFens;

    for (const auto& testCase : perftTestCases)
    {
        // Each position appears at multiple perft depths.
        // Test make/undo only once per unique FEN.
        if (std::find(
                testedFens.begin(),
                testedFens.end(),
                testCase.fen
            ) != testedFens.end())
        {
            continue;
        }

        testedFens.push_back(testCase.fen);

        std::cout << "\n[INFO] Make/undo identity: "
                  << testCase.name << '\n';

        testMakeUndoIdentity(testCase.fen);
    }

    check(
        "Make/undo identity: all six perft positions covered",
        testedFens.size() == 6,
        "Expected 6 unique perft positions, found " +
            std::to_string(testedFens.size())
    );
}


void testFenConsistencyDuringPlay(unsigned int seed)
{
    const string startFen =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    Board board;

    if (!loadTestPosition(
            board,
            startFen,
            "FEN consistency: load starting position"))
    {
        return;
    }

    mt19937 rng(seed);
    const int targetPlies = 200;

    bool allConsistent = true;
    int pliesPlayed = 0;

    for (int ply = 0; ply < targetPlies; ++ply)
    {
        // Check the current position before making a move.
        const string fenBeforeMove = board.toFEN();

        Board reconstructed;
        string error;

        const bool loaded = reconstructed.loadFEN(
            fenBeforeMove,
            &error
        );

        if (!loaded)
        {
            check(
                "FEN consistency: current FEN loads",
                false,
                "Ply " + to_string(ply) + ": " + error
            );

            allConsistent = false;
            break;
        }

        const string reconstructedFen = reconstructed.toFEN();

        if (fenBeforeMove != reconstructedFen)
        {
            check(
                "FEN consistency: round-trip at ply " +
                    to_string(ply),
                false,
                "Before: " + fenBeforeMove +
                " | After: " + reconstructedFen
            );

            allConsistent = false;
            break;
        }

        vector<Move> legalMoves = generateLegalMoves(board);

        // Stop naturally if the game reaches mate or stalemate.
        if (legalMoves.empty())
            break;

        uniform_int_distribution<size_t> distribution(
            0,
            legalMoves.size() - 1
        );

        const Move selectedMove = legalMoves[distribution(rng)];

        board.makeMove(selectedMove);
        ++pliesPlayed;
    }

    // Also check the final position after the last move.
    if (allConsistent)
    {
        const string finalFen = board.toFEN();
        Board reconstructed;
        string error;

        const bool loaded = reconstructed.loadFEN(finalFen, &error);

        if (!loaded || reconstructed.toFEN() != finalFen)
        {
            check(
                "FEN consistency: final position round-trip",
                false,
                loaded
                    ? "Final FEN changed after round-trip: " + finalFen
                    : "Final FEN could not be loaded: " + error
            );

            allConsistent = false;
        }
    }

    check(
        "FEN consistency: all positions round-trip",
        allConsistent
    );

    check(
        "FEN consistency: played at least 100 plies",
        pliesPlayed >= 100,
        "Actual plies played: " + to_string(pliesPlayed)
    );
}

void testLegalSubsetOfPseudoLegal(const string& fen)
{
    Board board;

    if (!loadTestPosition(
            board,
            fen,
            "Legal subset: load position"))
    {
        return;
    }

    const vector<Move> pseudoLegalMoves =
        MoveGenerator::generateAllMoves(board);

    const vector<Move> legalMoves =
        generateLegalMoves(board);

    bool allLegalMovesArePseudoLegal = true;

    for (const Move& legalMove : legalMoves)
    {
        bool found = false;

        for (const Move& pseudoMove : pseudoLegalMoves)
        {
            if (moveToString(legalMove) == moveToString(pseudoMove))
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            check(
                "Legal subset: " + moveToString(legalMove)
                    + " exists in pseudo-legal moves",
                false,
                "Legal move was absent from the pseudo-legal list."
            );

            allLegalMovesArePseudoLegal = false;
        }
    }

    check(
        "Legal subset: every legal move is pseudo-legal",
        allLegalMovesArePseudoLegal
    );
}


void testNoSelfCheck(const string& fen)
{
    Board board;

    if (!loadTestPosition(
            board,
            fen,
            "No self-check: load position"))
    {
        return;
    }

    const Board original = board;
    const Colour movingColour =
        board.getSideToMove() == SideToMove::white
            ? Colour::white
            : Colour::black;

    const vector<Move> legalMoves = generateLegalMoves(board);

    bool allMovesSafe = true;
    bool allMovesUndone = true;

    for (const Move& move : legalMoves)
    {
        board.makeMove(move);

        // The moving side's king must not be in check afterward.
        const bool kingSafe = !isInCheck(board, movingColour);

        if (!kingSafe)
        {
            check(
                "No self-check: " + moveToString(move),
                false,
                "The move leaves the moving side's king in check."
            );

            allMovesSafe = false;
        }

        const bool undone = board.undoMove();

        if (!undone || !board.isSameAs(original))
        {
            check(
                "No self-check: undo restores position after " +
                    moveToString(move),
                false,
                "Undo failed or the original position was not restored."
            );

            allMovesUndone = false;
        }
    }

    check(
        "No self-check: every legal move keeps its king safe",
        allMovesSafe
    );

    check(
        "No self-check: all moves restore original position",
        allMovesUndone
    );
}


void testPieceCountRule(const string& fen)
{
    Board board;

    if (!loadTestPosition(
            board,
            fen,
            "Piece count: load position"))
    {
        return;
    }

    const vector<Move> legalMoves = generateLegalMoves(board);
    const int originalCount = countAllPieces(board);

    for (const Move& move : legalMoves)
    {
        Board testBoard = board;

        const int beforeCount = countAllPieces(testBoard);

        testBoard.makeMove(move);

        const int afterCount = countAllPieces(testBoard);

        const bool isCapture =
            (move.flags & MOVE_CAPTURE) != 0 ||
            (move.flags & MOVE_EN_PASSANT) != 0;

        const int expectedCount =
            isCapture ? beforeCount - 1 : beforeCount;

        checkEqual(
            "Piece count after " + moveToString(move),
            expectedCount,
            afterCount
        );
    }

    checkEqual(
        "Piece count: original board unchanged",
        originalCount,
        countAllPieces(board)
    );
}


void testSideAlternation(const string& fen)
{
    Board board;

    if (!loadTestPosition(
            board,
            fen,
            "Side alternation: load position"))
    {
        return;
    }

    const vector<Move> legalMoves = generateLegalMoves(board);
    const SideToMove originalSide = board.getSideToMove();

    const SideToMove expectedSide =
        originalSide == SideToMove::white
            ? SideToMove::black
            : SideToMove::white;

    for (const Move& move : legalMoves)
    {
        Board testBoard = board;
        testBoard.makeMove(move);

        check(
            "Side alternation after " + moveToString(move),
            testBoard.getSideToMove() == expectedSide,
            "Side to move did not switch after the move."
        );
    }

    // Ensure testing the moves did not alter the original board.
    check(
        "Side alternation: original side unchanged",
        board.getSideToMove() == originalSide
    );
}



bool randomPlayout(
    const string& startFen,
    int plies,
    unsigned int seed)
{
    Board board;

    if (!loadTestPosition(
            board,
            startFen,
            "Random playout: load starting position"))
    {
        return false;
    }

    mt19937 rng(seed);

    bool fenOK = true;
    bool legalSubsetOK = true;
    bool pieceCountOK = true;
    bool sideAlternationOK = true;
    bool kingSafetyOK = true;
    bool makeUndoOK = true;

    string failureDetails;
    int playedPlies = 0;

    for (int ply = 0; ply < plies; ++ply)
    {
        // Stop when the game has ended.
        const GameState state = getGameState(board);

        if (state == GameState::Checkmate ||
            state == GameState::Stalemate ||
            state == GameState::DrawInsufficient ||
            state == GameState::DrawFiftyMove ||
            state == GameState::DrawRepetition)
        {
            break;
        }

        // Invariant 1: FEN must survive a round trip.
        const string fen = board.toFEN();

        Board reconstructed;
        string error;

        if (!reconstructed.loadFEN(fen, &error) ||
            reconstructed.toFEN() != fen)
        {
            fenOK = false;

            if (failureDetails.empty())
            {
                failureDetails =
                    "FEN round-trip failed at ply " +
                    to_string(ply) + ": " + fen +
                    (error.empty() ? "" : " | " + error);
            }

            break;
        }

        // Generate both pseudo-legal and legal moves.
        const vector<Move> pseudoLegalMoves =
            MoveGenerator::generateAllMoves(board);

        const vector<Move> legalMoves =
            generateLegalMoves(board);

        if (legalMoves.empty())
            break;

        // Invariant 2: every legal move must be pseudo-legal.
        for (const Move& legalMove : legalMoves)
        {
            bool found = false;

            for (const Move& pseudoMove : pseudoLegalMoves)
            {
                if (moveToString(legalMove) ==
                    moveToString(pseudoMove))
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                legalSubsetOK = false;

                if (failureDetails.empty())
                {
                    failureDetails =
                        "Legal move missing from pseudo-legal list "
                        "at ply " + to_string(ply) + ": " +
                        moveToString(legalMove);
                }

                break;
            }
        }

        // Pick a reproducible random legal move.
        uniform_int_distribution<size_t> distribution(
            0, legalMoves.size() - 1
        );

        const Move move = legalMoves[distribution(rng)];

        // Invariant 3: making and undoing a move must restore
        // the exact original board position.
        const Board original = board;

        board.makeMove(move);

        const bool undoSucceeded = board.undoMove();

        const bool restored =
            undoSucceeded &&
            board.isSameAs(original) &&
            board.toFEN() == original.toFEN();

        if (!restored)
        {
            makeUndoOK = false;

            if (failureDetails.empty())
            {
                failureDetails =
                    "Make/undo identity failed at ply " +
                    to_string(ply) + " for move " +
                    moveToString(move) +
                    (undoSucceeded
                        ? ": board position was not restored"
                        : ": undoMove() returned false");
            }

            // Do not continue from a potentially corrupted position.
            break;
        }

        // Undo succeeded. Replay the move so the random game
        // continues along its selected sequence.
        const SideToMove oldSide = board.getSideToMove();

        const Colour movingColour =
            oldSide == SideToMove::white
                ? Colour::white
                : Colour::black;

        const int piecesBefore = countAllPieces(board);

        board.makeMove(move);
        ++playedPlies;

        // Invariant 4: side to move must alternate.
        const SideToMove expectedSide =
            oldSide == SideToMove::white
                ? SideToMove::black
                : SideToMove::white;

        if (board.getSideToMove() != expectedSide)
        {
            sideAlternationOK = false;

            if (failureDetails.empty())
            {
                failureDetails =
                    "Side did not alternate after " +
                    moveToString(move);
            }
        }

        // Invariant 5: piece count changes only on captures.
        const bool isCapture =
            (move.flags & MOVE_CAPTURE) != 0 ||
            (move.flags & MOVE_EN_PASSANT) != 0;

        const int expectedPieces =
            isCapture ? piecesBefore - 1 : piecesBefore;

        if (countAllPieces(board) != expectedPieces)
        {
            pieceCountOK = false;

            if (failureDetails.empty())
            {
                failureDetails =
                    "Unexpected piece count after " +
                    moveToString(move);
            }
        }

        // Invariant 6: the moving side's king must be safe.
        if (isInCheck(board, movingColour))
        {
            kingSafetyOK = false;

            if (failureDetails.empty())
            {
                failureDetails =
                    "Move left the moving side in check: " +
                    moveToString(move);
            }
        }
    }

    // Also check the final position's FEN.
    const string finalFen = board.toFEN();
    Board finalBoard;
    string finalError;

    if (!finalBoard.loadFEN(finalFen, &finalError) ||
        finalBoard.toFEN() != finalFen)
    {
        fenOK = false;

        if (failureDetails.empty())
        {
            failureDetails =
                "Final FEN round-trip failed: " + finalFen +
                (finalError.empty() ? "" : " | " + finalError);
        }
    }

    const string prefix =
        "Random playout seed " + to_string(seed);

    check(prefix + ": FEN consistency",
          fenOK, failureDetails);

    check(prefix + ": legal subset of pseudo-legal",
          legalSubsetOK, failureDetails);

    check(prefix + ": piece-count invariant",
          pieceCountOK, failureDetails);

    check(prefix + ": side alternation",
          sideAlternationOK, failureDetails);

    check(prefix + ": king safety",
          kingSafetyOK, failureDetails);

    check(prefix + ": make/undo identity",
          makeUndoOK, failureDetails);

    const bool allOK =
        fenOK &&
        legalSubsetOK &&
        pieceCountOK &&
        sideAlternationOK &&
        kingSafetyOK &&
        makeUndoOK;

    check(prefix + ": completed without invariant failure",
          allOK, failureDetails);

    cout << "[INFO] Random playout seed " << seed
         << ": " << playedPlies << " plies played.\n";

    return allOK;
}


void testRandomPlayouts(int count, unsigned int baseSeed)
{
    const vector<string> startPositions = {
        // Standard starting position
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",

        // Castling position
        "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",

        // En passant position
        "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1",

        // Promotion position
        "4k3/P7/8/8/8/8/8/4K3 w - - 0 1"
    };

    bool allPassed = true;

    for (int i = 0; i < count; ++i)
    {
        const string& fen =
            startPositions[i % startPositions.size()];

        const unsigned int seed =
            baseSeed + static_cast<unsigned int>(i);

        if (!randomPlayout(fen, 200, seed))
            allPassed = false;
    }

    check(
        "Random playouts: all games preserve invariants",
        allPassed
    );
}

//evaluation tests

void testEvaluationScoringConvention()
{
    // White and Black pieces must have identical values.
    checkEqual(
        "Piece value: white knight",
        pieceValue('N'),
        320
    );

    checkEqual(
        "Piece value: black knight",
        pieceValue('n'),
        320
    );

    check(
        "Piece values: white and black knights equal",
        pieceValue('N') == pieceValue('n')
    );

    // Empty squares and invalid characters have no value.
    checkEqual(
        "Piece value: empty square",
        pieceValue('.'),
        0
    );

    checkEqual(
        "Piece value: invalid character",
        pieceValue('X'),
        0
    );

    // Verify the remaining piece values.
    checkEqual("Piece value: pawn", pieceValue('P'), 100);
    checkEqual("Piece value: bishop", pieceValue('B'), 330);
    checkEqual("Piece value: rook", pieceValue('R'), 500);
    checkEqual("Piece value: queen", pieceValue('Q'), 900);
    checkEqual("Piece value: king", pieceValue('K'), 20000);

    // White's perspective: preserve the score.
    checkEqual(
        "Score conversion: White to move",
        toWhiteView(150, SideToMove::white),
        150
    );

    // Black's perspective: flip the sign.
    checkEqual(
        "Score conversion: Black to move",
        toWhiteView(150, SideToMove::black),
        -150
    );

    checkEqual(
        "Score conversion: negative Black score",
        toWhiteView(-250, SideToMove::black),
        250
    );

    checkEqual(
        "Score conversion: zero remains zero",
        toWhiteView(0, SideToMove::black),
        0
    );
}

void testMaterialEvaluation()
{
    Board board;

    // 1. Starting position: equal material.
    board.loadStartPosition();

    checkEqual(
        "Material: starting position is balanced",
        0,
        evalMaterial(board)
    );

    checkEqual(
        "Material: starting position White total",
        4000,
        materialOf(board, Colour::white)
    );

    checkEqual(
        "Material: starting position Black total",
        4000,
        materialOf(board, Colour::black)
    );

    // 2. Remove one black pawn from a7.
    board.setPiece(1, 0, EMPTY);

    checkEqual(
        "Material: removing Black pawn gives White +100",
        100,
        evalMaterial(board)
    );

    checkEqual(
        "Material: Black total after losing pawn",
        3900,
        materialOf(board, Colour::black)
    );

    // 3. Reset, then remove White's queen from d1.
    board.loadStartPosition();
    board.setPiece(7, 3, EMPTY);

    checkEqual(
        "Material: removing White queen gives -900",
        -900,
        evalMaterial(board)
    );

    checkEqual(
        "Material: White total after losing queen",
        3100,
        materialOf(board, Colour::white)
    );

    // 4. Kings must not contribute to material totals.
    board.clear();
    board.setPiece(0, 4, 'k');
    board.setPiece(7, 4, 'K');

    checkEqual(
        "Material: kings contribute zero",
        0,
        evalMaterial(board)
    );

    checkEqual(
        "Material: king-only White total",
        0,
        materialOf(board, Colour::white)
    );

    checkEqual(
        "Material: king-only Black total",
        0,
        materialOf(board, Colour::black)
    );
}

void testPieceSquareTables()
{
    const int mgPhase = EvalParams::MAX_PHASE;

    // Knight in the centre versus a corner, for White.
    check(
        "PST: White knight on d4 beats a1",
        pstValue(PieceType::n, Colour::white, 4, 3, mgPhase)
            > pstValue(PieceType::n, Colour::white, 7, 0, mgPhase)
    );

    // Knight in the centre versus a corner, for Black.
    check(
        "PST: Black knight on d4 beats a1",
        pstValue(PieceType::n, Colour::black, 4, 3, mgPhase)
            > pstValue(PieceType::n, Colour::black, 7, 0, mgPhase)
    );

    // Mirror a White knight from d4 to d5 for Black.
    checkEqual(
        "PST: mirrored knight square has same value",
        pstValue(PieceType::n, Colour::white, 4, 3, mgPhase),
        pstValue(PieceType::n, Colour::black, 3, 3, mgPhase)
    );

    checkEqual(
        "PST: mirrorRow(0)",
        7,
        mirrorRow(0)
    );

    checkEqual(
        "PST: mirrorRow(7)",
        0,
        mirrorRow(7)
    );

    // Invalid coordinates should safely return zero.
    checkEqual(
        "PST: invalid row returns zero",
        0,
        pstValue(PieceType::n, Colour::white, -1, 3, mgPhase)
    );

    // King uses the middlegame table at full phase.
    checkEqual(
        "PST: king uses middlegame table at full phase",
        EvalParams::KING_MG_PST[7][4],
        pstValue(PieceType::k, Colour::white, 7, 4, mgPhase)
    );

    // King uses the endgame table at zero phase.
    checkEqual(
        "PST: king uses endgame table at zero phase",
        EvalParams::KING_EG_PST[7][4],
        pstValue(PieceType::k, Colour::white, 7, 4, 0)
    );
}

void testEvalPieceSquare()
{
    const int phase = EvalParams::MAX_PHASE;

    // Test 1: Starting position should be positionally balanced.
    Board board;
    board.loadStartPosition();

    checkEqual(
        "Eval PST: starting position is balanced",
        0,
        evalPieceSquare(board, phase)
    );

    // Test 2: A central White knight should score better than one on a1.
    board.clear();
    board.setPiece(4, 3, 'N'); // d4
    const int whiteCenterScore = evalPieceSquare(board, phase);

    board.clear();
    board.setPiece(7, 0, 'N'); // a1
    const int whiteCornerScore = evalPieceSquare(board, phase);

    check(
        "Eval PST: White knight prefers d4 over a1",
        whiteCenterScore > whiteCornerScore
    );

    // Test 3: The same preference should hold for Black.
    board.clear();
    board.setPiece(4, 3, 'n'); // d4
    const int blackCenterScore = evalPieceSquare(board, phase);

    board.clear();
    board.setPiece(7, 0, 'n'); // a1
    const int blackCornerScore = evalPieceSquare(board, phase);

    // Scores are White-relative, so a better Black position
    // produces a lower (more negative) score.
    check(
        "Eval PST: Black knight prefers d4 over a1",
        blackCenterScore < blackCornerScore
    );

    // Test 4: Vertically mirror the board and swap piece colours.
    Board original;
    original.clear();

    original.setPiece(7, 4, 'K');
    original.setPiece(0, 4, 'k');
    original.setPiece(4, 3, 'N');
    original.setPiece(2, 6, 'p');
    original.setPiece(6, 1, 'b');
    original.setPiece(1, 5, 'R');

    Board mirrored;
    mirrored.clear();

    for (int r = 0; r < 8; ++r)
    {
        for (int c = 0; c < 8; ++c)
        {
            const char piece = original.getPiece(r, c);

            if (!isValid(piece))
                continue;

            char swappedPiece;

            if (isWhite(piece))
                swappedPiece = static_cast<char>(
                    tolower(static_cast<unsigned char>(piece))
                );
            else
                swappedPiece = static_cast<char>(
                    toupper(static_cast<unsigned char>(piece))
                );

            mirrored.setPiece(7 - r, c, swappedPiece);
        }
    }

    const int originalScore = evalPieceSquare(original, phase);
    const int mirroredScore = evalPieceSquare(mirrored, phase);

    checkEqual(
        "Eval PST: mirrored position flips score",
        -originalScore,
        mirroredScore
    );
}

void testGamePhase()
{
    Board board;

    // 1. Starting position: full middlegame phase.
    board.loadStartPosition();

    checkEqual(
        "Game phase: starting position",
        24,
        gamePhase(board)
    );

    // 2. Kings only: endgame phase.
    board.clear();
    board.setPiece(0, 4, 'k');
    board.setPiece(7, 4, 'K');

    checkEqual(
        "Game phase: kings only",
        0,
        gamePhase(board)
    );

    // 3. Remove both queens from the starting position.
    board.loadStartPosition();
    board.setPiece(0, 3, EMPTY);
    board.setPiece(7, 3, EMPTY);

    checkEqual(
        "Game phase: both queens removed",
        16,
        gamePhase(board)
    );

    // 4. Phase is capped at MAX_PHASE.
    board.clear();

    // Add more pieces than a normal position can contain.
    for (int r = 0; r < 8; ++r)
    {
        for (int c = 0; c < 8; ++c)
            board.setPiece(r, c, 'q');
    }

    checkEqual(
        "Game phase: capped at maximum",
        EvalParams::MAX_PHASE,
        gamePhase(board)
    );

    // 5. Tapering at full middlegame phase.
    checkEqual(
        "Taper: full middlegame",
        100,
        taper(100, 20, 24)
    );

    // 6. Tapering at full endgame phase.
    checkEqual(
        "Taper: full endgame",
        20,
        taper(100, 20, 0)
    );

    // 7. Tapering halfway between both phases.
    checkEqual(
        "Taper: halfway blend",
        60,
        taper(100, 20, 12)
    );

    // 8. Out-of-range phase values are clamped.
    checkEqual(
        "Taper: phase above maximum",
        100,
        taper(100, 20, 100)
    );

    checkEqual(
        "Taper: negative phase",
        20,
        taper(100, 20, -5)
    );
}


void testPawnInfo()
{
    Board board;

    // 1. Starting position: eight pawns per colour,
    // with one pawn on each file.
    board.loadStartPosition();

    const PawnInfo whiteStart =
        buildPawnInfo(board, Colour::white);

    const PawnInfo blackStart =
        buildPawnInfo(board, Colour::black);

    checkEqual(
        "Pawn info: White has eight pawns",
        8,
        [&]() {
            int count = 0;
            for (int file = 1; file <= 8; ++file)
                count += whiteStart.pawnsOnFile[file];
            return count;
        }()
    );

    checkEqual(
        "Pawn info: Black has eight pawns",
        8,
        [&]() {
            int count = 0;
            for (int file = 1; file <= 8; ++file)
                count += blackStart.pawnsOnFile[file];
            return count;
        }()
    );

    checkEqual(
        "Pawn info: starting White has no doubled pawns",
        0,
        isDoubledPawnCount(whiteStart)
    );

    checkEqual(
        "Pawn info: starting Black has no doubled pawns",
        0,
        isDoubledPawnCount(blackStart)
    );

    // 2. White has doubled pawns on the c-file.
    board.loadFEN(
        "4k3/8/8/8/8/2P5/2P5/4K3 w - - 0 1"
    );

    const PawnInfo whiteDoubled =
        buildPawnInfo(board, Colour::white);

    checkEqual(
        "Pawn info: two White pawns on c-file",
        2,
        whiteDoubled.pawnsOnFile[3]
    );

    checkEqual(
        "Pawn info: doubled White pawn count",
        1,
        isDoubledPawnCount(whiteDoubled)
    );

    checkEqual(
        "Pawn info: most advanced White c-pawn row",
        5,
        whiteDoubled.mostAdvancedRow[3]
    );

    // 3. Black advances toward larger row numbers.
    board.loadFEN(
        "4k3/2p5/2p5/8/8/8/8/4K3 w - - 0 1"
    );

    const PawnInfo blackDoubled =
        buildPawnInfo(board, Colour::black);

    checkEqual(
        "Pawn info: two Black pawns on c-file",
        2,
        blackDoubled.pawnsOnFile[3]
    );

    checkEqual(
        "Pawn info: doubled Black pawn count",
        1,
        isDoubledPawnCount(blackDoubled)
    );

    checkEqual(
        "Pawn info: most advanced Black c-pawn row",
        2,
        blackDoubled.mostAdvancedRow[3]
    );

    // 4. No pawns: every file has count zero and row -1.
    board.clear();

    const PawnInfo emptyInfo =
        buildPawnInfo(board, Colour::white);

    checkEqual(
        "Pawn info: empty board has no doubled pawns",
        0,
        isDoubledPawnCount(emptyInfo)
    );

    checkEqual(
        "Pawn info: empty a-file has no advanced row",
        -1,
        emptyInfo.mostAdvancedRow[1]
    );
}

void testIsolatedPawns()
{
    Board board;

    // White pawns on a2 and c2, with no pawn on b-file.
    board.loadFEN(
        "4k3/8/8/8/8/8/P1P5/4K3 w - - 0 1"
    );

    const PawnInfo whiteInfo =
        buildPawnInfo(board, Colour::white);

    checkEqual(
        "Isolated pawns: a-file pawn is isolated",
        true,
        isIsolated(whiteInfo, 1)
    );

    checkEqual(
        "Isolated pawns: c-file pawn is isolated",
        true,
        isIsolated(whiteInfo, 3)
    );

    checkEqual(
        "Isolated pawns: empty b-file is not isolated",
        false,
        isIsolated(whiteInfo, 2)
    );

    // Add a White pawn on b2: a- and c-file pawns
    // are no longer isolated.
    board.loadFEN(
        "4k3/8/8/8/8/8/PPP5/4K3 w - - 0 1"
    );

    const PawnInfo supportedInfo =
        buildPawnInfo(board, Colour::white);

    checkEqual(
        "Isolated pawns: a-file pawn supported by b-file",
        false,
        isIsolated(supportedInfo, 1)
    );

    checkEqual(
        "Isolated pawns: c-file pawn supported by b-file",
        false,
        isIsolated(supportedInfo, 3)
    );

    // Edge file: an h-file pawn checks only the g-file.
    board.loadFEN(
        "4k3/8/8/8/8/8/7P/4K3 w - - 0 1"
    );

    const PawnInfo edgeInfo =
        buildPawnInfo(board, Colour::white);

    checkEqual(
        "Isolated pawns: h-file pawn is isolated",
        true,
        isIsolated(edgeInfo, 8)
    );

    checkEqual(
        "Isolated pawns: invalid file zero",
        false,
        isIsolated(edgeInfo, 0)
    );

    checkEqual(
        "Isolated pawns: invalid file nine",
        false,
        isIsolated(edgeInfo, 9)
    );
}

void testPassedPawnDetection()
{
    Board board;

    // White pawn on d5, no enemy pawns ahead.
    board.loadFEN("4k3/8/8/3P4/8/8/8/4K3 w - - 0 1");

    checkEqual(
        "Passed pawn: White pawn on d5 is passed",
        true,
        isPassed(board, Colour::white, 3, 3)
    );

    // Black pawn on d7 blocks the White pawn.
    board.loadFEN("4k3/3p4/8/3P4/8/8/8/4K3 w - - 0 1");

    checkEqual(
        "Passed pawn: enemy pawn on same file blocks",
        false,
        isPassed(board, Colour::white, 3, 3)
    );

    // Enemy pawn on c7 blocks the White pawn.
    board.loadFEN("4k3/2p5/8/3P4/8/8/8/4K3 w - - 0 1");

    checkEqual(
        "Passed pawn: enemy pawn on adjacent left file blocks",
        false,
        isPassed(board, Colour::white, 3, 3)
    );

    // Enemy pawn on e7 blocks the White pawn.
    board.loadFEN("4k3/4p3/8/3P4/8/8/8/4K3 w - - 0 1");

    checkEqual(
        "Passed pawn: enemy pawn on adjacent right file blocks",
        false,
        isPassed(board, Colour::white, 3, 3)
    );

    // An enemy pawn behind the White pawn does not block it.
    board.loadFEN("4k3/8/8/3P4/8/3p4/8/4K3 w - - 0 1");

    checkEqual(
        "Passed pawn: enemy pawn behind White pawn does not block",
        true,
        isPassed(board, Colour::white, 3, 3)
    );

    // Black pawn on d4, with no enemy pawns ahead.
    board.loadFEN("4k3/8/8/8/3p4/8/8/4K3 b - - 0 1");

    checkEqual(
        "Passed pawn: Black pawn on d4 is passed",
        true,
        isPassed(board, Colour::black, 4, 3)
    );

    // White pawn on d6 blocks the Black pawn on d4.
    board.loadFEN("4k3/8/8/8/3p4/3P4/8/4K3 b - - 0 1");

    checkEqual(
        "Passed pawn: enemy pawn ahead blocks Black pawn",
        false,
        isPassed(board, Colour::black, 4, 3)
    );

    // Invalid coordinates must not be treated as passed pawns.
    checkEqual(
        "Passed pawn: invalid coordinates",
        false,
        isPassed(board, Colour::white, -1, 3)
    );

    // A square containing a non-pawn is not a passed pawn.
    board.loadFEN("4k3/8/8/3N4/8/8/8/4K3 w - - 0 1");

    checkEqual(
        "Passed pawn: knight is not a passed pawn",
        false,
        isPassed(board, Colour::white, 3, 3)
    );
}

void testPassedPawnBonus()
{
    // White pawn advancement.
    checkEqual(
        "Passed bonus: White rank 2",
        10,
        passedPawnBonus(Colour::white, 6, EvalParams::MAX_PHASE)
    );

    checkEqual(
        "Passed bonus: White rank 3",
        20,
        passedPawnBonus(Colour::white, 5, EvalParams::MAX_PHASE)
    );

    checkEqual(
        "Passed bonus: White rank 4",
        35,
        passedPawnBonus(Colour::white, 4, EvalParams::MAX_PHASE)
    );

    checkEqual(
        "Passed bonus: White rank 5",
        60,
        passedPawnBonus(Colour::white, 3, EvalParams::MAX_PHASE)
    );

    checkEqual(
        "Passed bonus: White rank 6",
        100,
        passedPawnBonus(Colour::white, 2, EvalParams::MAX_PHASE)
    );

    checkEqual(
        "Passed bonus: White rank 7",
        150,
        passedPawnBonus(Colour::white, 1, EvalParams::MAX_PHASE)
    );

    // Black uses the same bonuses from its own perspective.
    checkEqual(
        "Passed bonus: Black rank 2",
        10,
        passedPawnBonus(Colour::black, 1, EvalParams::MAX_PHASE)
    );

    checkEqual(
        "Passed bonus: Black rank 7",
        150,
        passedPawnBonus(Colour::black, 6, EvalParams::MAX_PHASE)
    );

    // Endgame bonus is 50% larger.
    checkEqual(
        "Passed bonus: endgame rank 7",
        225,
        passedPawnBonus(Colour::white, 1, 0)
    );

    // Halfway between middlegame and endgame.
    checkEqual(
        "Passed bonus: half-phase rank 7",
        187,
        passedPawnBonus(Colour::white, 1, EvalParams::MAX_PHASE / 2)
    );

    // Bonuses are zero for ranks 1 and 8.
    checkEqual(
        "Passed bonus: rank 1",
        0,
        passedPawnBonus(Colour::white, 7, EvalParams::MAX_PHASE)
    );

    checkEqual(
        "Passed bonus: rank 8",
        0,
        passedPawnBonus(Colour::white, 0, EvalParams::MAX_PHASE)
    );

    // Invalid row.
    checkEqual(
        "Passed bonus: invalid row",
        0,
        passedPawnBonus(Colour::white, -1, EvalParams::MAX_PHASE)
    );

    // Phase values are clamped.
    checkEqual(
        "Passed bonus: phase above maximum",
        150,
        passedPawnBonus(Colour::white, 1, EvalParams::MAX_PHASE + 10)
    );

    checkEqual(
        "Passed bonus: negative phase",
        225,
        passedPawnBonus(Colour::white, 1, -10)
    );
}

void testEvalPawnStructure()
{
    Board board;

    // 1. Kings only: no pawns, so score is zero.
    board.loadFEN("4k3/8/8/8/8/8/8/4K3 w - - 0 1");

    checkEqual(
        "Pawn structure: kings only",
        0,
        evalPawnStructure(board, EvalParams::MAX_PHASE)
    );

    // 2. Doubled White pawns on c-file.
    // Doubled: -12; isolated: -36; passed bonuses: +10.
    board.loadFEN("4k3/8/8/8/8/2P5/2P5/4K3 w - - 0 1");

    checkEqual(
        "Pawn structure: doubled White pawns",
        -18,
        evalPawnStructure(board, EvalParams::MAX_PHASE)
    );

    // 3. Doubled Black pawns on c-file.
    // Doubled: +12; isolated: +36; passed bonuses: -10.
    board.loadFEN("4k3/2p5/2p5/8/8/8/8/4K3 w - - 0 1");

    checkEqual(
        "Pawn structure: doubled Black pawns",
        18,
        evalPawnStructure(board, EvalParams::MAX_PHASE)
    );

    // 4. Two isolated White pawns on a2 and c2.
    // Both are passed; their bonuses are +10 each.
    board.loadFEN("4k3/8/8/8/8/8/P1P5/4K3 w - - 0 1");

    checkEqual(
        "Pawn structure: two isolated White pawns",
        -16,
        evalPawnStructure(board, EvalParams::MAX_PHASE)
    );

    // 5. Two isolated Black pawns on a2 and c2.
    // Both are passed; each receives a rank-7 bonus of 150.
    board.loadFEN("4k3/8/8/8/8/8/p1p5/4K3 w - - 0 1");

    checkEqual(
        "Pawn structure: two isolated Black pawns",
        -264,
        evalPawnStructure(board, EvalParams::MAX_PHASE)
    );

    // 6. White passed pawn on d5.
    // Passed bonus: +60; isolated penalty: -18.
    board.loadFEN("4k3/8/8/3P4/8/8/8/4K3 w - - 0 1");

    checkEqual(
        "Pawn structure: White passed pawn on d5",
        42,
        evalPawnStructure(board, EvalParams::MAX_PHASE)
    );

    // 7. Black passed pawn on d4.
    // Passed bonus: -60; isolated penalty: +18.
    board.loadFEN("4k3/8/8/8/3p4/8/8/4K3 b - - 0 1");

    checkEqual(
        "Pawn structure: Black passed pawn on d4",
        -42,
        evalPawnStructure(board, EvalParams::MAX_PHASE)
    );

    // 8. White passed pawn on d5 in the endgame.
    // Endgame bonus: +90; isolated penalty: -18.
    board.loadFEN("4k3/8/8/3P4/8/8/8/4K3 w - - 0 1");

    checkEqual(
        "Pawn structure: White passed pawn endgame",
        72,
        evalPawnStructure(board, 0)
    );

    // 9. Black pawn on d7 blocks White's passed pawn on d5.
    // Both pawns are isolated; their penalties cancel.
    // Black pawn on d7 also has a passed bonus of -150.
    board.loadFEN("4k3/3p4/8/3P4/8/8/8/4K3 w - - 0 1");

    checkEqual(
        "Pawn structure: blocked passed pawn",
        0,
        evalPawnStructure(board, EvalParams::MAX_PHASE)
    );
}

void testCountMobility()
{
    std::cout << "\n--- Count Mobility Tests ---\n";

    Board board;

    // A knight on d4 has 8 available destinations.
    board.clear();
    board.setPiece(4, 3, 'N'); // d4
    checkEqual("Knight d4 mobility", 8,
               countMobility(board, Square{4, 3}));

    // A knight on a1 has only 2 available destinations.
    board.clear();
    board.setPiece(7, 0, 'N'); // a1
    checkEqual("Knight a1 mobility", 2,
               countMobility(board, Square{7, 0}));

    // A friendly piece blocks and is not counted.
    board.clear();
    board.setPiece(4, 3, 'N'); // d4
    board.setPiece(2, 2, 'P'); // c6
    checkEqual("Knight excludes friendly square", 7,
               countMobility(board, Square{4, 3}));

    // An enemy piece can be captured and is counted.
    board.clear();
    board.setPiece(4, 3, 'N'); // d4
    board.setPiece(2, 2, 'p'); // c6
    checkEqual("Knight counts enemy capture", 8,
               countMobility(board, Square{4, 3}));

    // Bishop blocked by its own pawns.
    board.clear();
    board.setPiece(4, 3, 'B'); // d4
    board.setPiece(3, 2, 'P'); // c5
    board.setPiece(3, 4, 'P'); // e5
    board.setPiece(5, 2, 'P'); // c3
    board.setPiece(5, 4, 'P'); // e3
    const int boxedBishop =
        countMobility(board, Square{4, 3});

    // Open bishop.
    board.clear();
    board.setPiece(4, 3, 'B'); // d4
    const int openBishop =
        countMobility(board, Square{4, 3});

    checkEqual("Open bishop has greater mobility", true,
               openBishop > boxedBishop);

    // Start position has symmetric mobility.
    board.loadStartPosition();
    int whiteMobility = 0;
    int blackMobility = 0;

    for (int r = 0; r < 8; ++r)
    {
        for (int c = 0; c < 8; ++c)
        {
            const char piece = board.getPiece(r, c);

            if (!isValid(piece))
                continue;

            if (pieceType(piece) == PieceType::p ||
                pieceType(piece) == PieceType::k)
                continue;

            const int mobility =
                countMobility(board, Square{r, c});

            if (colourOf(piece) == Colour::white)
                whiteMobility += mobility;
            else
                blackMobility += mobility;
        }
    }

    checkEqual("Starting position mobility is symmetric",
               0, whiteMobility - blackMobility);

    // Pawns and kings have zero mobility by design.
    checkEqual("Pawn mobility is zero", 0,
               countMobility(board, Square{6, 4}));

    checkEqual("King mobility is zero", 0,
               countMobility(board, Square{7, 4}));
}

void testMobilityWeight()
{
    std::cout << "\n--- Mobility Weight Tests ---\n";

    checkEqual("Knight mobility weight", 4,
               mobilityWeight(PieceType::n));
    checkEqual("Bishop mobility weight", 5,
               mobilityWeight(PieceType::b));
    checkEqual("Rook mobility weight", 2,
               mobilityWeight(PieceType::r));
    checkEqual("Queen mobility weight", 1,
               mobilityWeight(PieceType::q));

    checkEqual("Pawn mobility weight", 0,
               mobilityWeight(PieceType::p));
    checkEqual("King mobility weight", 0,
               mobilityWeight(PieceType::k));

    checkEqual("Knight mobility baseline", 4,
               mobilityBaseline(PieceType::n));
    checkEqual("Bishop mobility baseline", 5,
               mobilityBaseline(PieceType::b));
    checkEqual("Rook mobility baseline", 7,
               mobilityBaseline(PieceType::r));
    checkEqual("Queen mobility baseline", 10,
               mobilityBaseline(PieceType::q));
}


void testEvalMobility()
{
    std::cout << "\n--- Evaluation Mobility Tests ---\n";

    Board board;

    // Empty board with only kings: no mobility score.
    board.clear();
    board.setPiece(7, 4, 'K');
    board.setPiece(0, 4, 'k');

    checkEqual("Kings only mobility", 0,
               evalMobility(board));

    // A centrally placed knight should score better than a corner knight.
    board.clear();
    board.setPiece(4, 3, 'N'); // d4
    board.setPiece(0, 4, 'k');

    const int centralKnightScore = evalMobility(board);

    board.clear();
    board.setPiece(7, 0, 'N'); // a1
    board.setPiece(0, 4, 'k');

    const int cornerKnightScore = evalMobility(board);

    checkEqual("Central knight scores higher", 1,
               centralKnightScore > cornerKnightScore ? 1 : 0);

    // An open bishop should score better than a bishop blocked by friendly pawns.
    board.clear();
    board.setPiece(4, 3, 'B'); // d4
    board.setPiece(0, 4, 'k');

    const int openBishopScore = evalMobility(board);

    board.clear();
    board.setPiece(4, 3, 'B'); // d4
    board.setPiece(3, 2, 'P'); // c5
    board.setPiece(3, 4, 'P'); // e5
    board.setPiece(5, 2, 'P'); // c3
    board.setPiece(5, 4, 'P'); // e3
    board.setPiece(0, 4, 'k');

    const int boxedBishopScore = evalMobility(board);

    checkEqual("Open bishop scores higher", 1,
               openBishopScore > boxedBishopScore ? 1 : 0);

    // The starting position must have equal mobility for both sides.
    board.loadStartPosition();

    checkEqual("Starting position mobility is symmetric",
               0, evalMobility(board));
}


void testPawnShieldPenalty()
{
    std::cout << "\n--- Pawn Shield Penalty Tests ---\n";

    Board board;

    // White king on g1 with a full f2-g2-h2 pawn shield.
    board.clear();
    board.setPiece(7, 6, 'K'); // g1
    board.setPiece(0, 4, 'k'); // e8
    board.setPiece(6, 5, 'P'); // f2
    board.setPiece(6, 6, 'P'); // g2
    board.setPiece(6, 7, 'P'); // h2

    const int fullShield =
        pawnShieldPenalty(board, Colour::white);

    // Remove the f2-g2-h2 pawns.
    board.setPiece(6, 5, EMPTY);
    board.setPiece(6, 6, EMPTY);
    board.setPiece(6, 7, EMPTY);

    const int missingShield =
        pawnShieldPenalty(board, Colour::white);

    checkEqual("Full shield has zero penalty", 0, fullShield);
    checkEqual("Missing shield has higher penalty", 1,
               missingShield > fullShield ? 1 : 0);

    // A central king should not receive a pawn-shield penalty.
    board.clear();
    board.setPiece(4, 4, 'K'); // e4
    board.setPiece(0, 4, 'k'); // e8

    checkEqual("Central king has no shield penalty", 0,
               pawnShieldPenalty(board, Colour::white));

    // Black king on g8 with a full f7-g7-h7 pawn shield.
    board.clear();
    board.setPiece(0, 6, 'k'); // g8
    board.setPiece(7, 4, 'K'); // e1
    board.setPiece(1, 5, 'p'); // f7
    board.setPiece(1, 6, 'p'); // g7
    board.setPiece(1, 7, 'p'); // h7

    checkEqual("Black full shield has zero penalty", 0,
               pawnShieldPenalty(board, Colour::black));
}

void testOpenFilePenalty()
{
    std::cout << "\n--- Open File Penalty Tests ---\n";

    Board board;

    // Friendly pawns on the king's file and neighboring files.
    board.clear();
    board.setPiece(7, 6, 'K'); // g1
    board.setPiece(0, 4, 'k'); // e8
    board.setPiece(6, 5, 'P'); // f2
    board.setPiece(6, 6, 'P'); // g2
    board.setPiece(6, 7, 'P'); // h2

    const int shieldedPenalty =
        openFilePenalty(board, Colour::white);

    checkEqual("Friendly pawns protect nearby files", 0,
               shieldedPenalty);

    // Remove the three friendly pawns: nearby files become fully open.
    board.setPiece(6, 5, EMPTY);
    board.setPiece(6, 6, EMPTY);
    board.setPiece(6, 7, EMPTY);

    const int openPenalty =
        openFilePenalty(board, Colour::white);

    checkEqual("Fully open files increase penalty", 1,
               openPenalty > shieldedPenalty ? 1 : 0);

    // Add an enemy rook on the g-file.
    board.setPiece(3, 6, 'r');

    const int rookPenalty =
        openFilePenalty(board, Colour::white);

    checkEqual("Enemy rook increases open-file penalty", 1,
               rookPenalty > openPenalty ? 1 : 0);

    
    // Compare a fully open g-file against a semi-open g-file.
    // Remove the enemy rook, then put an enemy pawn on the g-file.
    board.setPiece(3, 6, EMPTY);
    const int fullyOpenWithoutRook =
        openFilePenalty(board, Colour::white);

    board.setPiece(4, 6, 'p');
    const int semiOpenPenalty =
        openFilePenalty(board, Colour::white);

    checkEqual("Semi-open file has lower penalty than fully open",
               1, semiOpenPenalty < fullyOpenWithoutRook ? 1 : 0);

    // A king on a corner should not access files outside the board.
    board.clear();
    board.setPiece(7, 0, 'K'); // a1
    board.setPiece(0, 4, 'k'); // e8

    const int cornerPenalty =
        openFilePenalty(board, Colour::white);

    checkEqual("Corner king open-file penalty is nonnegative",
               1, cornerPenalty >= 0 ? 1 : 0);
}


void testKingZoneSquares()
{
    std::cout << "\n--- King Zone Squares Tests ---\n";

    // A king in the center has 9 squares: itself plus 8 neighbors.
    const std::vector<Square> centerZone =
        kingZoneSquares(Square{4, 4}, Colour::white);

    checkEqual("Center king zone size", 9,
               static_cast<int>(centerZone.size()));

    // Every returned square must be valid.
    bool allValid = true;

    for (const Square& square : centerZone)
    {
        if (!inBounds(square.r, square.c))
            allValid = false;
    }

    checkEqual("Center zone squares are in bounds", 1,
               allValid ? 1 : 0);

    // A corner king has only 4 squares: itself plus 3 neighbors.
    const std::vector<Square> cornerZone =
        kingZoneSquares(Square{7, 0}, Colour::white);

    checkEqual("Corner king zone size", 4,
               static_cast<int>(cornerZone.size()));

    // An invalid king square returns an empty zone.
    const std::vector<Square> invalidZone =
        kingZoneSquares(Square{-1, -1}, Colour::white);

    checkEqual("Invalid king square gives empty zone", 0,
               static_cast<int>(invalidZone.size()));

    // Verify that no duplicate squares are returned.
    bool noDuplicates = true;

    for (std::size_t i = 0; i < centerZone.size(); ++i)
    {
        for (std::size_t j = i + 1; j < centerZone.size(); ++j)
        {
            if (centerZone[i].r == centerZone[j].r &&
                centerZone[i].c == centerZone[j].c)
            {
                noDuplicates = false;
            }
        }
    }

    checkEqual("King zone contains no duplicates", 1,
               noDuplicates ? 1 : 0);
}


void testAttackerWeight()
{
    std::cout << "\n--- Attacker Weight Tests ---\n";

    checkEqual("Queen attacker weight", 5,
               attackerWeight(PieceType::q));
    checkEqual("Rook attacker weight", 3,
               attackerWeight(PieceType::r));
    checkEqual("Bishop attacker weight", 2,
               attackerWeight(PieceType::b));
    checkEqual("Knight attacker weight", 2,
               attackerWeight(PieceType::n));
    checkEqual("Pawn attacker weight", 1,
               attackerWeight(PieceType::p));
    checkEqual("King attacker weight", 0,
               attackerWeight(PieceType::k));
}

void testKingAttackScore()
{
    std::cout << "\n--- King Attack Score Tests ---\n";

    Board board;

    // No enemy attacking pieces near the white king.
    board.clear();
    board.setPiece(7, 6, 'K'); // g1
    board.setPiece(0, 0, 'k'); // a8

    checkEqual("No nearby attackers gives zero penalty", 0,
               kingAttackScore(board, Colour::white));

    // A rook attacking the king zone should create a penalty.
    board.clear();
    board.setPiece(7, 6, 'K'); // g1
    board.setPiece(0, 0, 'k'); // a8
    board.setPiece(4, 6, 'r'); // g4

    const int rookAttack =
        kingAttackScore(board, Colour::white);

    checkEqual("Rook attack creates penalty", 1,
               rookAttack > 0 ? 1 : 0);

    // A queen attacker should create at least as much penalty as a rook.
    board.setPiece(4, 6, EMPTY);
    board.setPiece(4, 5, 'q'); // f4

    const int queenAttack =
        kingAttackScore(board, Colour::white);

    checkEqual("Queen attack is at least as dangerous as rook", 1,
               queenAttack >= rookAttack ? 1 : 0);

    // A blocking friendly piece must prevent a rook attack through it.
    board.clear();
    board.setPiece(7, 6, 'K'); // g1
    board.setPiece(0, 0, 'k'); // a8
    board.setPiece(4, 6, 'r'); // g4
    board.setPiece(5, 6, 'P'); // g3 blocks rook's ray

    checkEqual("Blocking piece stops rook attack", 0,
               kingAttackScore(board, Colour::white));
}

void testEvalKingSafety()
{
    std::cout << "\n--- King Safety Evaluation Tests ---\n";

    // Test 1: Symmetric starting position.
    {
        Board board;
        board.loadStartPosition();

        checkEqual(
            "Starting position has balanced king safety",
            0,
            evalKingSafety(board)
        );
    }

    // Test 2: Unsafe White king.
    {
        Board board;
        board.clear();

        board.setPiece(7, 6, 'K'); // White king g1
        board.setPiece(0, 0, 'k'); // Black king a8
        board.setPiece(4, 6, 'r'); // Black rook g4

        int score = evalKingSafety(board);

        checkEqual(
            "Unsafe White king receives a negative safety score",
            true,
            score < 0
        );
    }

    // Test 3: Unsafe Black king.
    {
        Board board;
        board.clear();

        board.setPiece(7, 0, 'K'); // White king a1
        board.setPiece(0, 6, 'k'); // Black king g8
        board.setPiece(3, 6, 'R'); // White rook g5

        int score = evalKingSafety(board);

        checkEqual(
            "Unsafe Black king receives a positive safety score",
            true,
            score > 0
        );
    }
}

void testEvalBishopPair()
{
    std::cout << "\n--- Bishop Pair Tests ---\n";

    // White has two bishops; Black has a bishop and a knight.
    {
        Board board;
        board.clear();

        board.setPiece(7, 4, 'K');
        board.setPiece(0, 4, 'k');

        board.setPiece(7, 2, 'B');
        board.setPiece(5, 5, 'B');

        board.setPiece(0, 2, 'b');
        board.setPiece(0, 5, 'n');

        checkEqual(
            "White bishop pair receives +40",
            40,
            evalBishopPair(board)
        );
    }

    // Black has the bishop pair.
    {
        Board board;
        board.clear();

        board.setPiece(7, 4, 'K');
        board.setPiece(0, 4, 'k');

        board.setPiece(0, 2, 'b');
        board.setPiece(2, 5, 'b');

        checkEqual(
            "Black bishop pair receives -40",
            -40,
            evalBishopPair(board)
        );
    }

    // Both sides have two bishops, so the bonuses cancel.
    {
        Board board;
        board.clear();

        board.setPiece(7, 4, 'K');
        board.setPiece(0, 4, 'k');

        board.setPiece(7, 2, 'B');
        board.setPiece(7, 5, 'B');

        board.setPiece(0, 2, 'b');
        board.setPiece(0, 5, 'b');

        checkEqual(
            "Bishop-pair bonuses cancel when both sides have them",
            0,
            evalBishopPair(board)
        );
    }
}

void testEvalRookFiles()
{
    std::cout << "\n--- Rook File Activity Tests ---\n";

    // An open file has no pawns of either colour.
    {
        Board board;
        board.clear();

        board.setPiece(7, 4, 'K');
        board.setPiece(0, 4, 'k');
        board.setPiece(4, 3, 'R');

        checkEqual(
            "File without pawns is open",
            true,
            isOpenFile(board, 3)
        );

        checkEqual(
            "White rook on open file gets +25",
            25,
            evalRookFiles(board)
        );
    }

    // White rook on a semi-open file: no White pawn, but a Black pawn.
    {
        Board board;
        board.clear();

        board.setPiece(7, 4, 'K');
        board.setPiece(0, 4, 'k');
        board.setPiece(4, 3, 'R');
        board.setPiece(2, 3, 'p');

        checkEqual(
            "White has a semi-open file",
            true,
            isSemiOpenFile(board, 3, Colour::white)
        );

        checkEqual(
            "White rook on semi-open file gets +10",
            10,
            evalRookFiles(board)
        );
    }

    // A White pawn on the rook's file means it is neither open nor semi-open.
    {
        Board board;
        board.clear();

        board.setPiece(7, 4, 'K');
        board.setPiece(0, 4, 'k');
        board.setPiece(4, 3, 'R');
        board.setPiece(6, 3, 'P');

        checkEqual(
            "File with a friendly pawn is not semi-open for White",
            false,
            isSemiOpenFile(board, 3, Colour::white)
        );

        checkEqual(
            "White rook behind its own pawn gets no file bonus",
            0,
            evalRookFiles(board)
        );
    }

    // Black's rook-file bonuses should be negative from White's perspective.
    {
        Board board;
        board.clear();

        board.setPiece(7, 4, 'K');
        board.setPiece(0, 4, 'k');
        board.setPiece(3, 5, 'r');

        checkEqual(
            "Black rook on open file gets -25",
            -25,
            evalRookFiles(board)
        );
    }
}

void testEvalRookOnSeventh()
{
    std::cout << "\n--- Rook on Seventh Rank Tests ---\n";

    // White rook on rank 7.
    {
        Board board;
        board.clear();

        board.setPiece(7, 4, 'K');
        board.setPiece(0, 4, 'k');
        board.setPiece(1, 3, 'R');

        checkEqual(
            "White rook on rank 7 gets +20",
            20,
            evalRookOnSeventh(board)
        );
    }

    // White rook on rank 3 should not receive the bonus.
    {
        Board board;
        board.clear();

        board.setPiece(7, 4, 'K');
        board.setPiece(0, 4, 'k');
        board.setPiece(5, 3, 'R');

        checkEqual(
            "White rook on rank 3 gets no seventh-rank bonus",
            0,
            evalRookOnSeventh(board)
        );
    }

    // Black rook on rank 2.
    {
        Board board;
        board.clear();

        board.setPiece(7, 4, 'K');
        board.setPiece(0, 4, 'k');
        board.setPiece(6, 3, 'r');

        checkEqual(
            "Black rook on rank 2 gets -20",
            -20,
            evalRookOnSeventh(board)
        );
    }

    // Two White rooks on rank 7 receive two bonuses.
    {
        Board board;
        board.clear();

        board.setPiece(7, 4, 'K');
        board.setPiece(0, 4, 'k');
        board.setPiece(1, 1, 'R');
        board.setPiece(1, 6, 'R');

        checkEqual(
            "Two White rooks on rank 7 get +40",
            40,
            evalRookOnSeventh(board)
        );
    }
}

void testEvalActivity()
{
    std::cout << "\n--- Piece Activity Tests ---\n";

    // White has a bishop pair, an open-file rook,
    // and a rook on the seventh rank.
    {
        Board board;
        board.clear();

        board.setPiece(7, 4, 'K'); // White king e1
        board.setPiece(0, 4, 'k'); // Black king e8

        board.setPiece(7, 2, 'B'); // White bishop
        board.setPiece(5, 5, 'B'); // White bishop

        board.setPiece(4, 0, 'R'); // White rook on open a-file
        board.setPiece(1, 3, 'R'); // White rook on rank 7

        checkEqual(
            "Combined activity score is +110",
            110,
            evalActivity(board)
        );
    }

    // Black has a bishop pair and a rook on rank 2.
    {
        Board board;
        board.clear();

        board.setPiece(7, 4, 'K');
        board.setPiece(0, 4, 'k');

        board.setPiece(0, 2, 'b');
        board.setPiece(2, 5, 'b');

        board.setPiece(6, 3, 'r');

        checkEqual(
            "Black activity score is -85",
            -85,
            evalActivity(board)
        );
    }

    // Both sides have bishop pairs and open-file rooks.
    {
        Board board;
        board.clear();

        board.setPiece(7, 4, 'K');
        board.setPiece(0, 4, 'k');

        board.setPiece(7, 2, 'B');
        board.setPiece(7, 5, 'B');

        board.setPiece(0, 2, 'b');
        board.setPiece(0, 5, 'b');

        board.setPiece(4, 0, 'R');
        board.setPiece(3, 7, 'r');

        checkEqual(
            "Symmetric activity bonuses cancel",
            0,
            evalActivity(board)
        );
    }
}

void testEvaluationIntegration()
{
    Board board;
    board.loadStartPosition();

    // 1. Starting position should be symmetric before tempo.
    checkEqual(
        "Starting position has zero White-perspective evaluation",
        0,
        evaluateWhite(board)
    );

    // 2. White to move: evaluation includes the tempo bonus.
    checkEqual(
        "White-to-move evaluation includes tempo",
        evaluateWhite(board) + EvalParams::TEMPO_BONUS,
        evaluate(board)
    );

    // 3. Black to move: evaluation is from Black's perspective.
    const string blackToMoveFen =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1";

    checkEqual(
        "Black-to-move FEN loads",
        true,
        board.loadFEN(blackToMoveFen)
    );

    checkEqual(
        "Black-to-move evaluation flips White score and adds tempo",
        -evaluateWhite(board) + EvalParams::TEMPO_BONUS,
        evaluate(board)
    );

    // 4. Evaluation terms should be consistent with their sum.
    const EvalTerms terms = getEvalTerms(board);

    checkEqual(
        "Evaluation terms sum to White total",
        terms.material +
            terms.pieceSquare +
            terms.pawnStructure +
            terms.mobility +
            terms.kingSafety +
            terms.activity,
        terms.totalWhite
    );
}

void runRulesTestsWithPlayouts(int randomPlayoutCount)
{
    // Step 5: Existing game-rule tests
    testStartPosition();
    testPawnMovement();
    testKnightMovement();
    testBishopMovement();
    testRookMovement();
    testQueenMovement();
    testKingMovement();

    testPins();
    testCastling();
    testEnPassant();
    testPromotion();

    testCheck();
    testCheckmate();
    testStalemate();

    testDrawRules();
    testGameStateStrings();
    testRegressions();

    // Step 6.1: Make/undo identity
    testMakeUndoIdentityAll();

    // Step 6.2: Deep make/undo
    testDeepUndo(20261018u);

    // Step 6.3: FEN consistency during random play
    testFenConsistencyDuringPlay(20261018u);

    // Step 6.4: Legal moves must be pseudo-legal
    testLegalSubsetOfPseudoLegal(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    );
    testLegalSubsetOfPseudoLegal(
        "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1"
    );
    testLegalSubsetOfPseudoLegal(
        "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1"
    );
    testLegalSubsetOfPseudoLegal(
        "4k3/P7/8/8/8/8/8/4K3 w - - 0 1"
    );

    // Step 6.5: No self-check
    testNoSelfCheck(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    );
    testNoSelfCheck(
        "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1"
    );
    testNoSelfCheck(
        "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1"
    );
    testNoSelfCheck(
        "4k3/P7/8/8/8/8/8/4K3 w - - 0 1"
    );
    testNoSelfCheck(
        "4r1k1/8/8/8/8/8/4R3/4K3 w - - 0 1"
    );

    // Step 6.6: Piece-count invariants
    testPieceCountRule(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    );
    testPieceCountRule(
        "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1"
    );
    testPieceCountRule(
        "4k3/P7/8/8/8/8/8/4K3 w - - 0 1"
    );

    // Step 6.7: Side-to-move alternation
    testSideAlternation(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    );
    testSideAlternation(
        "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1"
    );
    testSideAlternation(
        "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1"
    );
    testSideAlternation(
        "4k3/P7/8/8/8/8/8/4K3 w - - 0 1"
    );

    // Step 6.8: Random playouts
    testRandomPlayouts(randomPlayoutCount, 20261018u);

    //phase6 tests
    testEvaluationScoringConvention();
    testMaterialEvaluation();
    testPieceSquareTables();
    testEvalPieceSquare();
    testGamePhase();
    testPawnInfo();
    testIsolatedPawns();
    testPassedPawnDetection();
    testPassedPawnBonus();
    testEvalPawnStructure();
    testCountMobility();
    testMobilityWeight();
    testEvalMobility();
    testPawnShieldPenalty();
    testOpenFilePenalty();
    testKingZoneSquares();
    testAttackerWeight();
    testKingAttackScore();
    testEvalKingSafety();
    testEvalBishopPair();
    testEvalRookFiles();
    testEvalRookOnSeventh();
    testEvalActivity();
    testEvaluationIntegration();
}


void runRulesTestsFast()
{
    runRulesTestsWithPlayouts(8);
}

void runRulesTestsSlow()
{
    runRulesTestsWithPlayouts(50);
}

void runRulesTestsGate()
{
    runRulesTestsWithPlayouts(2000);
}

