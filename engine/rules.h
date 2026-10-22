
#pragma once

#include "Board.h"
#include <string>

enum class GameState
{
    Playing,
    Check,
    Checkmate,
    Stalemate,
    DrawInsufficient,
    DrawFiftyMove,
    DrawRepetition
};

GameState getGameState(Board& board);

bool isCheckmate(Board& board);

bool isStalemate(Board& board);

string gameStateToString(GameState state);

bool isInsufficientMaterial(const Board& board);
bool isFiftyMoveDraw(const Board& board);

int repetitionCount(const Board& board);

bool isThreefoldRepetition(const Board& board);