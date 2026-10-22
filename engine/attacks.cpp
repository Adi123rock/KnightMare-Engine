
#include "attacks.h"

// Private helper functions for attack detection.
namespace
{
    bool pieceBelongsTo(char piece, Colour colour)
    {
        if (piece == EMPTY) return false;
        if (colour == Colour::white) return piece >= 'A' && piece <= 'Z';
        return piece >= 'a' && piece <= 'z';
    }

    // Look backwards for attacking pawns.
    bool attackedByPawn(const Board& board,Square target,Colour byColour)
    {
        int pawnRow = (byColour == Colour::white)? target.r + 1: target.r - 1;
        char pawn = (byColour == Colour::white) ? 'P' : 'p';
        for (int dc : {-1, 1})
        {
            int pawnCol = target.c + dc;
            if (inBounds(pawnRow, pawnCol) &&
                board.getPiece(pawnRow, pawnCol) == pawn) return true;
        }
        return false;
    }

    bool attackedByKnight(const Board& board,Square target,Colour byColour)
    {
        constexpr int directions[8][2] =
        {
            {-2, -1}, {-2, 1},
            {-1, -2}, {-1, 2},
            {1, -2},  {1, 2},
            {2, -1},  {2, 1}
        };
        char knight = (byColour == Colour::white) ? 'N' : 'n';
        for (const auto& direction : directions)
        {
            int r = target.r + direction[0];
            int c = target.c + direction[1];

            if (inBounds(r, c) && board.getPiece(r, c) == knight) return true;
        }

        return false;
    }

    bool attackedByKing(const Board& board,Square target,Colour byColour)
    {
        char king = (byColour == Colour::white) ? 'K' : 'k';
        for (int dr = -1; dr <= 1; dr++){
            for (int dc = -1; dc <= 1; dc++){
                if (dr == 0 && dc == 0) continue;
                int r = target.r + dr,c = target.c + dc;
                if (inBounds(r, c) && board.getPiece(r, c) == king) return true;
            }
        }
        return false;
    }


    bool attackedBySlider(const Board& board,Square target,Colour byColour,
        const int directions[][2],int count,char pieceA,char pieceB)
    {
        // Convert the requested piece types to the attacker's case.
        if (byColour == Colour::black)
        {
            pieceA = (char)(pieceA - 'A' + 'a');
            pieceB = (char)(pieceB - 'A' + 'a');
        }

        for (int i = 0; i < count; i++)
        {
            int r = target.r + directions[i][0];
            int c = target.c + directions[i][1];
            while (inBounds(r, c))
            {
                char piece = board.getPiece(r, c);
                if (piece != EMPTY)
                {
                    if (pieceBelongsTo(piece, byColour) &&
                        (piece == pieceA || piece == pieceB)) return true;
                    // Stop at the first occupied square.
                    break;
                }
                r += directions[i][0],c += directions[i][1];
            }
        }
        return false;
    }

}

Colour opposite(Colour colour)
{
    return (colour == Colour::white)? Colour::black: Colour::white;
}

Square findKing(const Board& board, Colour colour)
{
    return board.getKingSquare(colour);
}



// Main public function.
bool isSquareAttacked(const Board& board,Square target,Colour byColour)
{
    if (!inBounds(target.r, target.c)) return false;

    if (attackedByPawn(board, target, byColour)) return true;

    if (attackedByKnight(board, target, byColour)) return true;

    if (attackedByKing(board, target, byColour)) return true;

    constexpr int diagonalDirections[4][2] =
    {
        {-1, -1}, {-1, 1},
        {1, -1},  {1, 1}
    };

    if (attackedBySlider(board,target,byColour,diagonalDirections,4,'B','Q'))
    {
        return true;
    }

    constexpr int straightDirections[4][2] =
    {
        {-1, 0}, {1, 0},
        {0, -1}, {0, 1}
    };

    if (attackedBySlider(board, target, byColour,straightDirections, 4, 'R', 'Q'))
    {
        return true;
    }

    return false;
}


bool isInCheck(const Board& board, Colour colour)
{
    Square kingSquare = findKing(board, colour);

    // A missing king means there is no check to report.
    if (!inBounds(kingSquare.r, kingSquare.c)) return false;

    return isSquareAttacked(board,kingSquare,opposite(colour));
}
