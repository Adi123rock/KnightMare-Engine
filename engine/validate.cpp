
#include "validate.h"

#include "attacks.h"

#include <cstdlib>
#include <string>

using namespace std;

namespace
{
    bool fail(string* errorOut, const string& message)
    {
        if (errorOut != nullptr)
            *errorOut = message;

        return false;
    }

    bool isWhitePiece(char piece)
    {
        return piece >= 'A' && piece <= 'Z';
    }

    bool isBlackPiece(char piece)
    {
        return piece >= 'a' && piece <= 'z';
    }

    bool isKingAt(const Board& board, int r, int c, char king)
    {
        return board.getPiece(r, c) == king;
    }

    bool isRookAt(const Board& board, int r, int c, char rook)
    {
        return board.getPiece(r, c) == rook;
    }
}

int countPiece(const Board& board, char piece)
{
    int count = 0;

    for (int r = 0; r < 8; ++r)
    {
        for (int c = 0; c < 8; ++c)
        {
            if (board.getPiece(r, c) == piece)
                ++count;
        }
    }

    return count;
}

bool hasExactlyOneKingPerSide(const Board& board, string* errorOut)
{
    if (countPiece(board, 'K') != 1)
        return fail(errorOut, "Position must contain exactly one white king.");

    if (countPiece(board, 'k') != 1)
        return fail(errorOut, "Position must contain exactly one black king.");

    return true;
}

bool kingsNotAdjacent(const Board& board, string* errorOut)
{
    Square whiteKing = findKing(board, Colour::white);
    Square blackKing = findKing(board, Colour::black);

    if (whiteKing.r < 0 || blackKing.r < 0)
        return fail(errorOut, "Cannot check king adjacency: a king is missing.");

    int rowDistance = abs(whiteKing.r - blackKing.r);
    int colDistance = abs(whiteKing.c - blackKing.c);

    if (rowDistance <= 1 && colDistance <= 1)
        return fail(errorOut, "The two kings cannot be adjacent.");

    return true;
}

bool noPawnsOnBackRanks(const Board& board, string* errorOut)
{
    for (int c = 0; c < 8; ++c)
    {
        if (board.getPiece(0, c) == 'P' ||
            board.getPiece(0, c) == 'p' ||
            board.getPiece(7, c) == 'P' ||
            board.getPiece(7, c) == 'p')
        {
            return fail(errorOut, "Pawns cannot occupy rank 1 or rank 8.");
        }
    }

    return true;
}

bool pieceCountsReasonable(const Board& board, string* errorOut)
{
    int whitePieces = 0;
    int blackPieces = 0;

    int whitePawns = countPiece(board, 'P');
    int blackPawns = countPiece(board, 'p');

    for (int r = 0; r < 8; ++r)
    {
        for (int c = 0; c < 8; ++c)
        {
            char piece = board.getPiece(r, c);

            if (piece == EMPTY)
                continue;

            if (isWhitePiece(piece))
                ++whitePieces;
            else if (isBlackPiece(piece))
                ++blackPieces;
        }
    }

    if (whitePawns > 8)
        return fail(errorOut, "White cannot have more than eight pawns.");

    if (blackPawns > 8)
        return fail(errorOut, "Black cannot have more than eight pawns.");

    if (whitePieces > 16)
        return fail(errorOut, "White cannot have more than sixteen pieces.");

    if (blackPieces > 16)
        return fail(errorOut, "Black cannot have more than sixteen pieces.");

    return true;
}

bool opponentNotInCheck(const Board& board, string* errorOut)
{
    Colour opponent =
        board.getSideToMove() == SideToMove::white
            ? Colour::black
            : Colour::white;

    if (isInCheck(board, opponent))
    {
        return fail(
            errorOut,
            "The side that just moved cannot leave its own king in check."
        );
    }

    return true;
}

bool castlingRightsConsistent(const Board& board, string* errorOut)
{
    Castling rights = board.getCastlingRights();

    if (rights.wk)
    {
        if (!isKingAt(board, 7, 4, 'K') ||
            !isRookAt(board, 7, 7, 'R'))
        {
            return fail(
                errorOut,
                "White kingside castling requires a white king on e1 and rook on h1."
            );
        }
    }

    if (rights.wq)
    {
        if (!isKingAt(board, 7, 4, 'K') ||
            !isRookAt(board, 7, 0, 'R'))
        {
            return fail(
                errorOut,
                "White queenside castling requires a white king on e1 and rook on a1."
            );
        }
    }

    if (rights.bk)
    {
        if (!isKingAt(board, 0, 4, 'k') ||
            !isRookAt(board, 0, 7, 'r'))
        {
            return fail(
                errorOut,
                "Black kingside castling requires a black king on e8 and rook on h8."
            );
        }
    }

    if (rights.bq)
    {
        if (!isKingAt(board, 0, 4, 'k') ||
            !isRookAt(board, 0, 0, 'r'))
        {
            return fail(
                errorOut,
                "Black queenside castling requires a black king on e8 and rook on a8."
            );
        }
    }

    return true;
}

bool enPassantConsistent(const Board& board, string* errorOut)
{
    Square ep = board.getEnPassantSquare();

    // A negative row indicates that there is no en-passant target.
    if (ep.r == -1 && ep.c == -1)
        return true;

    if (!inBounds(ep.r, ep.c))
        return fail(errorOut, "En-passant target is outside the board.");

    // The target square must be empty.
    if (board.getPiece(ep.r, ep.c) != EMPTY)
        return fail(errorOut, "En-passant target square must be empty.");

    if (board.getSideToMove() == SideToMove::white)
    {
        // Black just pushed a pawn from rank 7 to rank 5.
        // FEN target is on rank 6.
        if (ep.r != 2)
            return fail(errorOut, "White to move requires an en-passant target on rank 6.");

        if (board.getPiece(3, ep.c) != 'p')
            return fail(errorOut, "Expected the black double-pushed pawn on rank 5.");

        if (board.getPiece(1, ep.c) != EMPTY)
            return fail(errorOut, "The black pawn's original square must be empty.");
    }
    else
    {
        // White just pushed a pawn from rank 2 to rank 4.
        // FEN target is on rank 3.
        if (ep.r != 5)
            return fail(errorOut, "Black to move requires an en-passant target on rank 3.");

        if (board.getPiece(4, ep.c) != 'P')
            return fail(errorOut, "Expected the white double-pushed pawn on rank 4.");

        if (board.getPiece(6, ep.c) != EMPTY)
            return fail(errorOut, "The white pawn's original square must be empty.");
    }

    return true;
}

bool validatePosition(const Board& board, string* errorOut)
{
    if (errorOut != nullptr)
        errorOut->clear();

    if (!hasExactlyOneKingPerSide(board, errorOut))
        return false;

    if (!kingsNotAdjacent(board, errorOut))
        return false;

    if (!noPawnsOnBackRanks(board, errorOut))
        return false;

    if (!pieceCountsReasonable(board, errorOut))
        return false;

    if (!opponentNotInCheck(board, errorOut))
        return false;

    if (!castlingRightsConsistent(board, errorOut))
        return false;

    if (!enPassantConsistent(board, errorOut))
        return false;

    return true;
}
