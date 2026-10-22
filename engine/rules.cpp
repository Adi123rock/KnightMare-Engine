
#include "rules.h"

#include "attacks.h"
#include "legal.h"

namespace
{
    Colour sideToColour(SideToMove side)
    {
        return side == SideToMove::white? Colour::white: Colour::black;
    }
}

bool isCheckmate(Board& board)
{
    if (!generateLegalMoves(board).empty()) return false;

    return isInCheck(board, sideToColour(board.getSideToMove()));
}

bool isStalemate(Board& board)
{
    if (!generateLegalMoves(board).empty()) return false;

    return !isInCheck(board, sideToColour(board.getSideToMove()));
}

GameState getGameState(Board& board)
{
    Colour side = sideToColour(board.getSideToMove());

    // Checkmate and stalemate take priority over draw rules.
    vector<Move> legalMoves = generateLegalMoves(board);

    if (legalMoves.empty())
    {
        if (isInCheck(board, side)) return GameState::Checkmate;

        return GameState::Stalemate;
    }

    if (isThreefoldRepetition(board)) return GameState::DrawRepetition;

    if (isInsufficientMaterial(board)) return GameState::DrawInsufficient;

    // 100 half-moves = 50 moves by each side without
    // a pawn move or capture. This is a claimable draw rule.
    if (isFiftyMoveDraw(board)) return GameState::DrawFiftyMove;

    if (isInCheck(board, side)) return GameState::Check;

    return GameState::Playing;
}

string gameStateToString(GameState state)
{
    switch (state)
    {
        case GameState::Playing:
            return "Playing";

        case GameState::Check:
            return "Check";

        case GameState::Checkmate:
            return "Checkmate";

        case GameState::Stalemate:
            return "Stalemate";

        case GameState::DrawInsufficient:
            return "Draw by insufficient material";

        case GameState::DrawFiftyMove:
            return "Draw by fifty-move rule";

        case GameState::DrawRepetition:
            return "Draw by repetition";
    }

    return "Unknown";
}


bool isInsufficientMaterial(const Board& board)
{
    int knights = 0;
    int bishops = 0;

    // int whiteBishops = 0;
    // int blackBishops = 0;

    int bishopSquareColour = -1;
    bool bishopsSameColour = true;

    for (int r = 0; r < 8; r++){
        for (int c = 0; c < 8; c++){
            char piece = board.getPiece(r, c);

            if (piece == EMPTY || piece == 'K' || piece == 'k') continue;

            char type = (char)tolower(piece);

            // Pawns, rooks and queens mean sufficient material.
            if (type == 'p' || type == 'r' || type == 'q') return false;

            if (type == 'n') knights++;
            else if (type == 'b')
            {
                bishops++;

                // if (piece == 'B') whiteBishops++;
                // else blackBishops++;

                int colour = (r + c) % 2;

                if (bishopSquareColour == -1) bishopSquareColour = colour;
                else if (bishopSquareColour != colour) bishopsSameColour = false;
            }
        }
    }
    // King vs King.
    if (knights == 0 && bishops == 0) return true;

    // King + one bishop/knight vs King.
    if (knights + bishops == 1) return true;

    // Only bishops remain, and every bishop is on the same
    // colour of square.
    if (knights == 0 && bishopsSameColour) return true;

    return false;
}


bool isFiftyMoveDraw(const Board& board)
{
    return board.getHalfMove() >= 100;
}

int repetitionCount(const Board& board)
{
    const vector<string>& keys = board.getPositionKeys();
    if (keys.empty()) return 0;

    const string currentKey = positionKey(board);

    int count = 0;
    int lastIndex = static_cast<int>(keys.size()) - 1;
    int halfmoveLimit = board.getHalfMove();

    // Only inspect positions with the same side to move.
    // A position with the same side to move occurs every two plies.
    for (int i = lastIndex, pliesBack = 0;
         i >= 0 && pliesBack <= halfmoveLimit;
         i -= 2, pliesBack += 2)
    {
        if (keys[i] == currentKey) count++;
    }
    return count;
}

bool isThreefoldRepetition(const Board& board)
{
    return repetitionCount(board) >= 3;
}