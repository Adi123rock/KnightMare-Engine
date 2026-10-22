
#pragma once

namespace EvalParams
{
    // All evaluation scores are measured in centipawns.
    // 100 centipawns = 1 pawn.

    constexpr int PAWN_VALUE   = 100;
    constexpr int KNIGHT_VALUE = 320;
    constexpr int BISHOP_VALUE = 330;
    constexpr int ROOK_VALUE   = 500;
    constexpr int QUEEN_VALUE  = 900;

    // The king has no material value in static evaluation.
    // This large value is reserved for future search/capture ordering.
    constexpr int KING_VALUE   = 20000;

    // Optional bonus for the side to move, in centipawns.
    constexpr int TEMPO_BONUS  = 10;

    // Mate scores will be handled by search, not static evaluation.
    constexpr int MATE_SCORE   = 30000;

    // Reserved for search bounds and initialization.
    constexpr int INFINITY_SCORE = 32000;
    constexpr int NO_SCORE       = 32001;

    // Piece-square table values are from White's perspective.
    //
    // Board layout:
    // Row 0 = rank 8
    // Row 1 = rank 7
    // ...
    // Row 7 = rank 1
    //
    // Black pieces will use a vertically mirrored row.

    inline constexpr int PAWN_PST[8][8] =
    {
        {  0,  0,  0,  0,  0,  0,  0,  0 },
        { 50, 50, 50, 50, 50, 50, 50, 50 },
        { 10, 10, 20, 30, 30, 20, 10, 10 },
        {  5,  5, 10, 25, 25, 10,  5,  5 },
        {  0,  0,  0, 20, 20,  0,  0,  0 },
        {  5, -5,-10,  0,  0,-10, -5,  5 },
        {  5, 10, 10,-20,-20, 10, 10,  5 },
        {  0,  0,  0,  0,  0,  0,  0,  0 }
    };

    inline constexpr int KNIGHT_PST[8][8] =
    {
        {-50,-40,-30,-30,-30,-30,-40,-50},
        {-40,-20,  0,  5,  5,  0,-20,-40},
        {-30,  5, 10, 15, 15, 10,  5,-30},
        {-30,  0, 15, 20, 20, 15,  0,-30},
        {-30,  5, 15, 20, 20, 15,  5,-30},
        {-30,  0, 10, 15, 15, 10,  0,-30},
        {-40,-20,  0,  0,  0,  0,-20,-40},
        {-50,-40,-30,-30,-30,-30,-40,-50}
    };

    inline constexpr int BISHOP_PST[8][8] =
    {
        {-20,-10,-10,-10,-10,-10,-10,-20},
        {-10,  5,  0,  0,  0,  0,  5,-10},
        {-10, 10, 10, 10, 10, 10, 10,-10},
        {-10,  0, 10, 10, 10, 10,  0,-10},
        {-10,  5,  5, 10, 10,  5,  5,-10},
        {-10,  0,  5, 10, 10,  5,  0,-10},
        {-10,  0,  0,  0,  0,  0,  0,-10},
        {-20,-10,-10,-10,-10,-10,-10,-20}
    };

    inline constexpr int ROOK_PST[8][8] =
    {
        {  0,  0,  0,  5,  5,  0,  0,  0},
        {  5, 10, 10, 10, 10, 10, 10,  5},
        { -5,  0,  0,  0,  0,  0,  0, -5},
        { -5,  0,  0,  0,  0,  0,  0, -5},
        { -5,  0,  0,  0,  0,  0,  0, -5},
        { -5,  0,  0,  0,  0,  0,  0, -5},
        { -5,  0,  0,  0,  0,  0,  0, -5},
        {  0,  0,  0,  5,  5,  0,  0,  0}
    };

    inline constexpr int QUEEN_PST[8][8] =
    {
        {-20,-10,-10, -5, -5,-10,-10,-20},
        {-10,  0,  5,  0,  0,  0,  0,-10},
        {-10,  5,  5,  5,  5,  5,  0,-10},
        { -5,  0,  5,  5,  5,  5,  0, -5},
        {  0,  0,  5,  5,  5,  5,  0, -5},
        {-10,  0,  5,  5,  5,  5,  0,-10},
        {-10,  0,  0,  0,  0,  0,  0,-10},
        {-20,-10,-10, -5, -5,-10,-10,-20}
    };

    // King middlegame table: encourage keeping the king sheltered.
    inline constexpr int KING_MG_PST[8][8] =
    {
        {-30,-40,-40,-50,-50,-40,-40,-30},
        {-30,-40,-40,-50,-50,-40,-40,-30},
        {-30,-40,-40,-50,-50,-40,-40,-30},
        {-30,-40,-40,-50,-50,-40,-40,-30},
        {-20,-30,-30,-40,-40,-30,-30,-20},
        {-10,-20,-20,-20,-20,-20,-20,-10},
        { 20, 20,  0,  0,  0,  0, 20, 20},
        { 20, 30, 10,  0,  0, 10, 30, 20}
    };

    // King endgame table: encourage centralization.
    inline constexpr int KING_EG_PST[8][8] =
    {
        {-50,-40,-30,-20,-20,-30,-40,-50},
        {-30,-20,-10,  0,  0,-10,-20,-30},
        {-30,-10, 20, 30, 30, 20,-10,-30},
        {-30,-10, 30, 40, 40, 30,-10,-30},
        {-30,-10, 30, 40, 40, 30,-10,-30},
        {-30,-10, 20, 30, 30, 20,-10,-30},
        {-30,-30,  0,  0,  0,  0,-30,-30},
        {-50,-30,-30,-30,-30,-30,-30,-50}
    };

    
    // Game phase weights.
    // Each side's starting material contributes:
    // 2 knights + 2 bishops + 2 rooks + 1 queen
    // = 2*1 + 2*1 + 2*2 + 1*4 = 12.
    // Both sides together contribute 24.

    constexpr int KNIGHT_PHASE_WEIGHT = 1;
    constexpr int BISHOP_PHASE_WEIGHT = 1;
    constexpr int ROOK_PHASE_WEIGHT   = 2;
    constexpr int QUEEN_PHASE_WEIGHT  = 4;

    constexpr int MAX_PHASE = 24;

    
    
    // --------------------------------------------------
    // Pawn structure
    // --------------------------------------------------

    constexpr int DOUBLED_PAWN_PENALTY = 12;
    constexpr int ISOLATED_PAWN_PENALTY = 18;

    inline constexpr int PASSED_PAWN_BONUS[8] =
    {
        0, 0, 10, 20, 35, 60, 100, 150
    };

    constexpr int PASSED_PAWN_ENDGAME_NUMERATOR = 3;
    constexpr int PASSED_PAWN_ENDGAME_DENOMINATOR = 2;

    // --------------------------------------------------
    // Mobility
    // --------------------------------------------------

    constexpr int KNIGHT_MOBILITY_WEIGHT = 4;
    constexpr int BISHOP_MOBILITY_WEIGHT = 5;
    constexpr int ROOK_MOBILITY_WEIGHT   = 2;
    constexpr int QUEEN_MOBILITY_WEIGHT  = 1;

    constexpr int KNIGHT_MOBILITY_BASELINE = 4;
    constexpr int BISHOP_MOBILITY_BASELINE = 5;
    constexpr int ROOK_MOBILITY_BASELINE   = 7;
    constexpr int QUEEN_MOBILITY_BASELINE  = 10;

    // --------------------------------------------------
    // King safety
    // --------------------------------------------------

    constexpr int MISSING_PAWN_SHIELD_PENALTY = 12;

    constexpr int SEMI_OPEN_KING_FILE_PENALTY = 10;
    constexpr int OPEN_KING_FILE_PENALTY = 20;
    constexpr int OPEN_KING_FILE_HEAVY_PIECE_PENALTY = 35;

    constexpr int KING_ATTACK_PAWN_WEIGHT   = 1;
    constexpr int KING_ATTACK_KNIGHT_WEIGHT = 2;
    constexpr int KING_ATTACK_BISHOP_WEIGHT = 2;
    constexpr int KING_ATTACK_ROOK_WEIGHT   = 3;
    constexpr int KING_ATTACK_QUEEN_WEIGHT  = 5;

    constexpr int KING_ATTACK_THRESHOLD_1 = 2;
    constexpr int KING_ATTACK_THRESHOLD_2 = 4;
    constexpr int KING_ATTACK_THRESHOLD_3 = 7;
    constexpr int KING_ATTACK_THRESHOLD_4 = 11;

    constexpr int KING_ATTACK_PENALTY_1 = 5;
    constexpr int KING_ATTACK_PENALTY_2 = 15;
    constexpr int KING_ATTACK_PENALTY_3 = 35;
    constexpr int KING_ATTACK_PENALTY_4 = 65;
    constexpr int KING_ATTACK_PENALTY_5 = 100;

    // --------------------------------------------------
    // Piece activity
    // --------------------------------------------------

    constexpr int BISHOP_PAIR_BONUS = 40;
    constexpr int ROOK_OPEN_FILE_BONUS = 25;
    constexpr int ROOK_SEMI_OPEN_FILE_BONUS = 10;
    constexpr int ROOK_SEVENTH_RANK_BONUS = 20;

}