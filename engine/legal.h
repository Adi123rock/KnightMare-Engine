
#pragma once

#include "Board.h"
#include "move.h"
#include <vector>

vector<Move> generateLegalMoves(Board& board);

bool isLegalMove(Board& board, const Move& move);


bool isCastlingLegal(const Board& board, const Move& move);
