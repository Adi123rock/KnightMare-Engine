#pragma once

#include "Board.h"
#include "Types.h"

// Return the opposite colour.
Colour opposite(Colour colour);

// Find the king of the requested colour.
// Returns Square{-1, -1} if the king is not found.
Square findKing(const Board& board, Colour colour);

bool isSquareAttacked(const Board& board,Square target,Colour byColour);


bool isInCheck(const Board& board, Colour colour);
