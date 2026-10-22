
#pragma once

#include <string>

#include "Board.h"

int countPiece(const Board& board, char piece);

bool hasExactlyOneKingPerSide(
    const Board& board,
    std::string* errorOut = nullptr
);

bool kingsNotAdjacent(
    const Board& board,
    std::string* errorOut = nullptr
);

bool noPawnsOnBackRanks(
    const Board& board,
    std::string* errorOut = nullptr
);

bool pieceCountsReasonable(
    const Board& board,
    std::string* errorOut = nullptr
);

bool opponentNotInCheck(
    const Board& board,
    std::string* errorOut = nullptr
);

bool castlingRightsConsistent(
    const Board& board,
    std::string* errorOut = nullptr
);

bool enPassantConsistent(
    const Board& board,
    std::string* errorOut = nullptr
);

bool validatePosition(
    const Board& board,
    std::string* errorOut = nullptr
);
