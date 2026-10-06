#include <iostream>
#include <string>
#include "Board.h"

using namespace std;

void check(bool condition, const string& testName)
{
    if (condition)
        cout << "PASS: " << testName << '\n';
    else
        cout << "FAIL: " << testName << '\n';
}

int main()
{
    Board board;

    // ========================================================
    // 1. CORRECT COORDINATE TESTS
    // ========================================================

    cout << "===== COORDINATE TESTS =====\n";

    // Your board uses:
    // r = 0 -> rank 8
    // r = 7 -> rank 1

    Square a1{7, 0};
    Square h8{0, 7};
    Square e1{7, 4};
    Square e8{0, 4};

    check(squareToName(a1) == "a1",
          "a1 -> a1");

    check(squareToName(h8) == "h8",
          "h8 -> h8");

    check(squareToName(e1) == "e1",
          "e1 -> e1");

    check(squareToName(e8) == "e8",
          "e8 -> e8");


    // 64-square round trip
    bool roundTrip = true;

    for (int r = 0; r < 8; r++)
    {
        for (int c = 0; c < 8; c++)
        {
            Square original{r, c};

            string name = squareToName(original);

            Square converted;

            if (!nameToSquare(name, converted))
            {
                roundTrip = false;
                continue;
            }

            if (original.r != converted.r ||
                original.c != converted.c)
            {
                roundTrip = false;
            }
        }
    }

    check(roundTrip,
          "64-square coordinate round trip");


    // ========================================================
    // 2. STARTING POSITION PIECE TESTS
    // ========================================================

    cout << "\n===== START POSITION TESTS =====\n";

    board.loadStartPosition();

    // r = 7 -> rank 1
    check(board.getPiece(7, 0) == 'R',
          "a1 = R");

    check(board.getPiece(7, 4) == 'K',
          "e1 = K");

    check(board.getPiece(7, 3) == 'Q',
          "d1 = Q");

    // r = 0 -> rank 8
    check(board.getPiece(0, 0) == 'r',
          "a8 = r");

    check(board.getPiece(0, 4) == 'k',
          "e8 = k");

    check(board.getPiece(0, 3) == 'q',
          "d8 = q");


    // ========================================================
    // 3. TRAILING '/' TEST
    // ========================================================

    cout << "\n===== TRAILING SLASH TEST =====\n";

    string validFEN = board.toFEN();

    // Add an extra '/' at the end of board placement
    string badFEN =
        "rnbqkbnr/pppppppp/8/8/8/8/"
        "PPPPPPPP/RNBQKBNR/ w KQkq - 0 1";

    check(!board.loadFEN(badFEN),
          "Reject trailing slash");

    // Board should remain unchanged
    check(board.toFEN() == validFEN,
          "Board unchanged after trailing slash");


    return 0;
}