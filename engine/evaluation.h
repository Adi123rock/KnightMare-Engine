
#pragma once
#include <algorithm>
#include <cctype>

#include "Board.h"
#include "attacks.h"
#include "eval_params.h"

// One shared table indexed by PieceType.
// The king's value is included for future search/capture ordering,
// but kings are excluded from material evaluation.
inline constexpr int PIECE_VALUES[] =
{
    EvalParams::PAWN_VALUE,    // PieceType::p
    EvalParams::KNIGHT_VALUE,  // PieceType::n
    EvalParams::BISHOP_VALUE,  // PieceType::b
    EvalParams::ROOK_VALUE,    // PieceType::r
    EvalParams::QUEEN_VALUE,   // PieceType::q
    EvalParams::KING_VALUE     // PieceType::k
};

// Converts a piece character into its PieceType.
inline PieceType pieceType(char piece)
{
    switch (tolower(static_cast<unsigned char>(piece)))
    {
        case 'p': return PieceType::p;
        case 'n': return PieceType::n;
        case 'b': return PieceType::b;
        case 'r': return PieceType::r;
        case 'q': return PieceType::q;
        default:  return PieceType::k;
    }
}

// Returns the material/search value of a piece.
// Empty squares and invalid characters return 0.
inline int pieceValue(char piece)
{
    if (!isValid(piece)) return 0;
    return PIECE_VALUES[static_cast<int>(pieceType(piece))];
}

// Converts a score from the side-to-move perspective
// into White's perspective.
inline int toWhiteView(int scoreForSideToMove,SideToMove sideToMove
)
{
    return sideToMove == SideToMove::white? scoreForSideToMove: -scoreForSideToMove;
}

// Returns the total material value belonging to one colour.
// Kings are deliberately excluded.
inline int materialOf(const Board& board, Colour colour)
{
    int material = 0;
    for (int r = 0; r < 8; ++r){
        for (int c = 0; c < 8; ++c){
            const char piece = board.getPiece(r, c);
            if (!isValid(piece)) continue;
            if (pieceType(piece) == PieceType::k) continue;
            if (colourOf(piece) == colour) material += pieceValue(piece);
        }
    }
    return material;
}

// Returns White's material minus Black's material.
// Positive means White has a material advantage.
// Negative means Black has a material advantage.
inline int evalMaterial(const Board& board)
{
    return materialOf(board, Colour::white)- materialOf(board, Colour::black);
}


// Vertically mirrors a board row.
// Row 0 (rank 8) becomes row 7 (rank 1), and vice versa.
inline int mirrorRow(int row) { return 7 - row; }

// Returns the positional bonus for a piece on a square.
//
// phase:
//   EvalParams::MAX_PHASE = full middlegame
//   0                     = full endgame
//
// White uses the table directly.
// Black uses the vertically mirrored row.
inline int pstValue(PieceType type,Colour colour,int row,int col,int phase)
{
    if (!inBounds(row, col)) return 0;
    // Clamp phase to its supported range.
    phase = std::max(0, std::min(phase, EvalParams::MAX_PHASE));
    const int tableRow =(colour == Colour::white) ? row : mirrorRow(row);
    switch (type)
    {
        case PieceType::p: return EvalParams::PAWN_PST[tableRow][col];
        case PieceType::n: return EvalParams::KNIGHT_PST[tableRow][col];
        case PieceType::b: return EvalParams::BISHOP_PST[tableRow][col];
        case PieceType::r: return EvalParams::ROOK_PST[tableRow][col];
        case PieceType::q: return EvalParams::QUEEN_PST[tableRow][col];
        case PieceType::k:
        {
            const int mg = EvalParams::KING_MG_PST[tableRow][col];
            const int eg = EvalParams::KING_EG_PST[tableRow][col];
            // Integer-weighted blend:
            // phase = 24 -> mg
            // phase = 0  -> eg
            return (mg *phase+eg*(EvalParams::MAX_PHASE -phase))/EvalParams::MAX_PHASE;
        }
    }

    return 0;
}


inline int evalPieceSquare(const Board& board, int phase)
{
    int score = 0;
    for (int r = 0; r < 8; ++r){
        for (int c = 0; c < 8; ++c){
            const char piece = board.getPiece(r, c);
            // Ignore empty squares and invalid characters.
            if (!isValid(piece)) continue;

            const PieceType type = pieceType(piece);
            const Colour colour = colourOf(piece);
            const int value = pstValue(type,colour,r,c,phase);
            if (colour == Colour::white) score += value;
            else score -= value;
        }
    }
    return score;
}


inline int gamePhase(const Board& board)
{
    int phase = 0;
    for (int r = 0; r < 8; ++r){
        for (int c = 0; c < 8; ++c){
            const char piece = board.getPiece(r, c);
            if (!isValid(piece))continue;

            switch (pieceType(piece))
            {
                case PieceType::n: phase += EvalParams::KNIGHT_PHASE_WEIGHT;
                    break;
                case PieceType::b: phase += EvalParams::BISHOP_PHASE_WEIGHT;
                    break;
                case PieceType::r: phase += EvalParams::ROOK_PHASE_WEIGHT;
                    break;
                case PieceType::q: phase += EvalParams::QUEEN_PHASE_WEIGHT;
                    break;
                // Pawns and kings do not contribute to game phase.
                case PieceType::p:
                case PieceType::k:
                    break;
            }
        }
    }
    // Never allow the phase to exceed the starting-position maximum.
    return std::min(phase, EvalParams::MAX_PHASE);
}

inline int taper(int mgScore, int egScore, int phase)
{
    // Keep phase within the supported range [0, MAX_PHASE].
    phase = std::max(0, std::min(phase, EvalParams::MAX_PHASE));
    return (mgScore*phase+egScore*(EvalParams::MAX_PHASE-phase)
        )/EvalParams::MAX_PHASE;
}


struct PawnInfo
{
    // Files are stored in slots 1..8:
    // slot 1 = file a, slot 2 = file b, ..., slot 8 = file h.
    // Slots 0 and 9 are padding for neighbour checks.
    int pawnsOnFile[10] = {};
    // Most advanced pawn's row on each file.
    // -1 means there is no pawn on that file.
    int mostAdvancedRow[10] =
    {
        -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1
    };
};

// Builds a pawn summary for one colour in a single board scan.
inline PawnInfo buildPawnInfo(const Board& board, Colour colour)
{
    PawnInfo info;
    for (int r = 0; r < 8; ++r){
        for (int c = 0; c < 8; ++c){
            const char piece = board.getPiece(r, c);
            if (!isValid(piece))continue;
            if (pieceType(piece) != PieceType::p)continue;
            if (colourOf(piece) != colour)continue;
            // Convert board column 0..7 to padded file slot 1..8.
            const int file = c + 1;
            ++info.pawnsOnFile[file];
            const int previousRow = info.mostAdvancedRow[file];
            if (previousRow == -1) info.mostAdvancedRow[file] = r;
            else if (colour == Colour::white && r < previousRow)
            {
                // White advances toward smaller row numbers.
                info.mostAdvancedRow[file] = r;
            }
            else if (colour == Colour::black && r > previousRow)
            {
                // Black advances toward larger row numbers.
                info.mostAdvancedRow[file] = r;
            }
        }
    }
    return info;
}

// Returns the number of extra pawns stacked on the same files.
// For example, three pawns on one file contribute two extra pawns.
inline int isDoubledPawnCount(const PawnInfo& info)
{
    int extraPawns = 0;
    for (int file = 1; file <= 8; ++file){
        if(info.pawnsOnFile[file]>1) extraPawns += info.pawnsOnFile[file] - 1;
    }
    return extraPawns;
}

inline bool isIsolated(const PawnInfo& info, int file)
{
    // Files are represented by slots 1..8.
    if (file < 1 || file > 8)return false;
    // An empty file cannot contain an isolated pawn.
    if (info.pawnsOnFile[file] == 0)return false;
    // No friendly pawns on either adjacent file.
    return info.pawnsOnFile[file - 1] == 0 && info.pawnsOnFile[file + 1] == 0;
}

inline bool isPassed(const Board& board,Colour colour,int row,int col)
{
    if (!inBounds(row, col)) return false;
    const char piece = board.getPiece(row, col);
    // The square must contain a pawn of the requested colour.
    if (!isValid(piece) || pieceType(piece) != PieceType::p 
    || colourOf(piece) != colour) return false;

    // Check the same file and both adjacent files.
    for (int enemyCol = col - 1; enemyCol <= col + 1; ++enemyCol){
        if (enemyCol < 0 || enemyCol >= 8)continue;
        for (int enemyRow = 0; enemyRow < 8; ++enemyRow){
            const char enemy = board.getPiece(enemyRow, enemyCol);
            if (!isValid(enemy) || pieceType(enemy) != PieceType::p
                || colourOf(enemy) == colour) continue;
            // White advances toward smaller row indices.
            // Black advances toward larger row indices.
            const bool enemyIsAhead =
                (colour == Colour::white && enemyRow < row)
                || (colour == Colour::black && enemyRow > row);

            if (enemyIsAhead) return false;
        }
    }
    return true;
}


inline int passedPawnBonus(Colour colour, int row, int phase)
{
    if (!inBounds(row, 0)) return 0;
    phase = std::max(0, std::min(phase, EvalParams::MAX_PHASE));
    const int relativeRank =(colour == Colour::white) ? 8 - row : row + 1;
    if (relativeRank < 2 || relativeRank > 7)return 0;
    const int baseBonus =EvalParams::PASSED_PAWN_BONUS[relativeRank];
    const int endgameBonus =
        baseBonus * EvalParams::PASSED_PAWN_ENDGAME_NUMERATOR
                  / EvalParams::PASSED_PAWN_ENDGAME_DENOMINATOR;
    return taper(baseBonus, endgameBonus, phase);
}


inline int evalPawnStructure(const Board& board, int phase)
{
    int score = 0;
    const PawnInfo whiteInfo = buildPawnInfo(board, Colour::white);
    const PawnInfo blackInfo = buildPawnInfo(board, Colour::black);
    // Doubled-pawn penalties.
    score -= isDoubledPawnCount(whiteInfo) * EvalParams::DOUBLED_PAWN_PENALTY;
    score += isDoubledPawnCount(blackInfo) * EvalParams::DOUBLED_PAWN_PENALTY;
    // Isolated-pawn penalties: one penalty per pawn on an
    // isolated file, not merely one penalty per file.
    for (int file = 1; file <= 8; ++file){
        if (isIsolated(whiteInfo,file))
            score-=whiteInfo.pawnsOnFile[file]*EvalParams::ISOLATED_PAWN_PENALTY;

        if (isIsolated(blackInfo, file))
            score += blackInfo.pawnsOnFile[file] * EvalParams::ISOLATED_PAWN_PENALTY;
    }
    // Passed-pawn bonuses.
    for (int row = 0; row < 8; ++row){
        for (int col = 0; col < 8; ++col){
            const char piece = board.getPiece(row, col);
            if (!isValid(piece) || pieceType(piece) != PieceType::p) continue;
            const Colour colour = colourOf(piece);
            if (isPassed(board, colour, row, col)){
                const int bonus = passedPawnBonus(colour, row, phase);
                if (colour == Colour::white) score += bonus;
                else score -= bonus;
            }
        }
    }
    return score;
}

inline int countMobility(const Board& board, Square from)
{
    if (!inBounds(from.r, from.c)) return 0;
    const char piece = board.getPiece(from.r, from.c);
    if (!isValid(piece)) return 0;
    const PieceType type = pieceType(piece);
    // Mobility is not evaluated for pawns or kings.
    if (type == PieceType::p || type == PieceType::k) return 0;
    const Colour colour = colourOf(piece);
    int count = 0;
    // Knight movement.
    if (type == PieceType::n){
        constexpr int knightMoves[8][2] =
        {
            {-2, -1}, {-2, 1},
            {-1, -2}, {-1, 2},
            { 1, -2}, { 1, 2},
            { 2, -1}, { 2, 1}
        };
        for (const auto& move : knightMoves){
            const int r = from.r + move[0];
            const int c = from.c + move[1];
            if (!inBounds(r, c)) continue;
            const char target = board.getPiece(r, c);
            if (isValid(target) && colourOf(target) == colour) continue;
            ++count;
        }
        return count;
    }
    // Bishop, rook, and queen sliding directions.
    constexpr int directions[8][2] =
    {
        {-1, -1}, {-1, 1},
        { 1, -1}, { 1, 1},
        {-1,  0}, { 1,  0},
        { 0, -1}, { 0,  1}
    };
    for (int d = 0; d < 8; ++d){
        const bool diagonal = (d < 4);
        // Bishops use diagonal directions only.
        if (type == PieceType::b && !diagonal) continue;
        // Rooks use straight directions only.
        if (type == PieceType::r && diagonal) continue;
        int r = from.r + directions[d][0];
        int c = from.c + directions[d][1];
        while (inBounds(r, c)){
            const char target = board.getPiece(r, c);
            if (isValid(target)){
                // Friendly pieces block the ray and are not counted.
                if (colourOf(target) == colour) break;
                // Enemy pieces can be captured, so count that square.
                ++count; break;
            }
            ++count;
            r += directions[d][0],c += directions[d][1];
        }
    }
    return count;
}


inline int mobilityWeight(PieceType type)
{
    switch (type)
    {
        case PieceType::n:
            return EvalParams::KNIGHT_MOBILITY_WEIGHT;
        case PieceType::b:
            return EvalParams::BISHOP_MOBILITY_WEIGHT;
        case PieceType::r:
            return EvalParams::ROOK_MOBILITY_WEIGHT;
        case PieceType::q:
            return EvalParams::QUEEN_MOBILITY_WEIGHT;
        case PieceType::p:
        case PieceType::k:
            return 0;
    }
    return 0;
}

inline int mobilityBaseline(PieceType type)
{
    switch (type)
    {
        case PieceType::n:
            return EvalParams::KNIGHT_MOBILITY_BASELINE;
        case PieceType::b:
            return EvalParams::BISHOP_MOBILITY_BASELINE;
        case PieceType::r:
            return EvalParams::ROOK_MOBILITY_BASELINE;
        case PieceType::q:
            return EvalParams::QUEEN_MOBILITY_BASELINE;
        case PieceType::p:
        case PieceType::k:
            return 0;
    }
    return 0;
}

inline int evalMobility(const Board& board)
{
    int score = 0;
    for (int r = 0; r < 8; ++r){
        for (int c = 0; c < 8; ++c){
            const char piece = board.getPiece(r, c);
            if (!isValid(piece)) continue;
            const PieceType type = pieceType(piece);
            // Pawn and king mobility are not evaluated.
            if (type == PieceType::p || type == PieceType::k) continue;
            const int mobility =countMobility(board, Square{r, c});
            const int pieceScore =(mobility - mobilityBaseline(type))*mobilityWeight(type);
            if (colourOf(piece) == Colour::white) score += pieceScore;
            else score -= pieceScore;
        }
    }
    return score;
}

inline int pawnShieldPenalty(const Board& board, Colour colour)
{
    // Find the king.
    Square kingSquare{-1, -1};
    const char kingPiece =(colour == Colour::white) ? 'K' : 'k';
    for (int r = 0; r < 8; ++r){
        for (int c = 0; c < 8; ++c){
            if (board.getPiece(r, c) == kingPiece){
                kingSquare = Square{r, c}; break;
            }
        }
        if (inBounds(kingSquare.r, kingSquare.c)) break;
    }
    if (!inBounds(kingSquare.r, kingSquare.c)) return 0;
    // Apply the pawn shield only when the king is near its
    // home rank, where castling and pawn shelter are relevant.
    if (colour == Colour::white && kingSquare.r < 6) return 0;
    if (colour == Colour::black && kingSquare.r > 1) return 0;
    const int forward = (colour == Colour::white) ? -1 : 1;
    const int shieldRow = kingSquare.r + forward;
    if (shieldRow < 0 || shieldRow >= 8) return 0;
    int penalty = 0;
    // Check the king's file and its adjacent files.
    for (int fileOffset = -1; fileOffset <= 1; ++fileOffset){
        const int file = kingSquare.c + fileOffset;
        if (file < 0 || file >= 8) continue;
        const char piece = board.getPiece(shieldRow, file);
        const char friendlyPawn = (colour == Colour::white) ? 'P' : 'p';
        if (piece != friendlyPawn) penalty += EvalParams::MISSING_PAWN_SHIELD_PENALTY;
    }
    return penalty;
}

inline int openFilePenalty(const Board& board, Colour colour)
{
    // Find the king.
    Square kingSquare{-1, -1};
    const char kingPiece =(colour == Colour::white) ? 'K' : 'k';
    for (int r = 0; r < 8; ++r){
        for (int c = 0; c < 8; ++c){
            if (board.getPiece(r, c) == kingPiece)
            {
                kingSquare = Square{r, c}; break;
            }
        }
        if (inBounds(kingSquare.r, kingSquare.c)) break;
    }
    if (!inBounds(kingSquare.r, kingSquare.c)) return 0;
    int penalty = 0;
    // Examine the king's file and its adjacent files.
    for (int fileOffset = -1; fileOffset <= 1; ++fileOffset){
        const int file = kingSquare.c + fileOffset;
        if (file < 0 || file >= 8) continue;
        bool friendlyPawnFound = false;
        bool enemyPawnFound = false;
        // Check the entire file for pawns.
        for (int row = 0; row < 8; ++row){
            const char piece = board.getPiece(row, file);
            if (piece==((colour==Colour::white)?'P' : 'p'))friendlyPawnFound = true;
            if (piece==((colour==Colour::white)?'p' : 'P')) enemyPawnFound = true;
        }
        // A friendly pawn means this file is not open for the enemy.
        if (friendlyPawnFound) continue;
        // Check whether the opponent has a rook or queen.
        bool enemyHeavyPieceFound = false;
        for (int row = 0; row < 8; ++row){
            const char piece = board.getPiece(row, file);
            if (!isValid(piece) || colourOf(piece) == colour) continue;
            const PieceType type = pieceType(piece);
            if (type == PieceType::r || type == PieceType::q) {
                enemyHeavyPieceFound = true; break;
            }
        }
        if (!enemyPawnFound){// Fully open file.
            penalty += enemyHeavyPieceFound? 
            EvalParams::OPEN_KING_FILE_HEAVY_PIECE_PENALTY: EvalParams::OPEN_KING_FILE_PENALTY;
        }
        else{// Semi-open from this colour's perspective.
            penalty += EvalParams::SEMI_OPEN_KING_FILE_PENALTY;
        }
    }
    return penalty;
}

inline std::vector<Square> kingZoneSquares(Square kingSquare,Colour colour)
{
    std::vector<Square> zone;
    if (!inBounds(kingSquare.r, kingSquare.c)) return zone;

    // Include the king's square and its eight neighbors.
    for (int dr = -1; dr <= 1; ++dr){
        for (int dc = -1; dc <= 1; ++dc){
            const int r = kingSquare.r + dr;
            const int c = kingSquare.c + dc;
            if (inBounds(r, c)) zone.push_back(Square{r, c});
        }
    }
    // Add one extra row forward, toward the opponent.
    const int forward = (colour == Colour::white) ? -1 : 1;
    const int extraRow = kingSquare.r + forward;
    if (inBounds(extraRow,kingSquare.c-1)) zone.push_back(Square{extraRow, kingSquare.c - 1});
    if (inBounds(extraRow, kingSquare.c)) zone.push_back(Square{extraRow, kingSquare.c});
    if (inBounds(extraRow, kingSquare.c + 1)) zone.push_back(Square{extraRow, kingSquare.c + 1});
    // Remove duplicates from the extra row, which overlaps
    // the king's neighboring squares in this initial definition.
    std::vector<Square> uniqueZone;
    for (const Square& square : zone){
        bool found = false;
        for (const Square& existing : uniqueZone){
            if (existing.r == square.r && existing.c == square.c){
                found = true; break;
            }
        }
        if (!found) uniqueZone.push_back(square);
    }
    return uniqueZone;
}

inline int attackerWeight(PieceType type)
{
    switch (type)
    {
        case PieceType::q:
            return EvalParams::KING_ATTACK_QUEEN_WEIGHT;
        case PieceType::r:
            return EvalParams::KING_ATTACK_ROOK_WEIGHT;
        case PieceType::b:
            return EvalParams::KING_ATTACK_BISHOP_WEIGHT;
        case PieceType::n:
            return EvalParams::KING_ATTACK_KNIGHT_WEIGHT;
        case PieceType::p:
            return EvalParams::KING_ATTACK_PAWN_WEIGHT;
        case PieceType::k:
            return 0;
    }
    return 0;
}

//helper

inline bool pieceAttacksSquare(const Board& board,Square from,Square target)
{
    if (!inBounds(from.r, from.c) || !inBounds(target.r, target.c)) return false;
    if (from.r == target.r && from.c == target.c) return false;
    const char piece = board.getPiece(from.r, from.c);
    if (!isValid(piece)) return false;
    const PieceType type = pieceType(piece);
    const Colour colour = colourOf(piece);
    const int dr = target.r - from.r;
    const int dc = target.c - from.c;
    const int absDr = std::abs(dr);
    const int absDc = std::abs(dc);
    if (type == PieceType::n)
        return (absDr == 2 && absDc == 1) || (absDr == 1 && absDc == 2);
    if (type == PieceType::p){
        const int forward = (colour == Colour::white) ? -1 : 1;
        return dr == forward && absDc == 1;
    }
    if (type == PieceType::k) return std::max(absDr, absDc) == 1;

    bool diagonal = absDr == absDc && absDr > 0;
    bool straight = (dr == 0) != (dc == 0);

    if (type == PieceType::b && !diagonal) return false;
    if (type == PieceType::r && !straight) return false;
    if (type == PieceType::q && !diagonal && !straight) return false;
    if (type != PieceType::b &&
        type != PieceType::r && type != PieceType::q) return false;
    const int stepR = (dr > 0) - (dr < 0);
    const int stepC = (dc > 0) - (dc < 0);
    int r = from.r + stepR;
    int c = from.c + stepC;
    while (r != target.r || c != target.c){
        if (!isEmpty(board.getPiece(r, c))) return false;
        r += stepR,c += stepC;
    }
    return true;
}


inline int kingAttackScore(const Board& board, Colour colour)
{
    Square kingSquare{-1, -1};

    const char kingPiece =(colour == Colour::white) ? 'K' : 'k';

    // Find the king.
    for (int r = 0; r < 8; ++r){
        for (int c = 0; c < 8; ++c){
            if (board.getPiece(r, c) == kingPiece)
            {
                kingSquare = Square{r, c}; break;
            }
        }

        if (inBounds(kingSquare.r, kingSquare.c)) break;
    }

    if (!inBounds(kingSquare.r, kingSquare.c)) return 0;

    const Colour enemyColour =
        (colour == Colour::white)? Colour::black: Colour::white;

    const std::vector<Square> zone =kingZoneSquares(kingSquare, colour);

    int totalWeight = 0;
    // Count the weights of enemy pieces attacking the king zone.
    for (int r = 0; r < 8; ++r){
        for (int c = 0; c < 8; ++c){
            const char piece = board.getPiece(r, c);

            if (!isValid(piece) || colourOf(piece) != enemyColour) continue;

            const PieceType type = pieceType(piece);

            // Adjacent enemy kings are not counted.
            if (type == PieceType::k) continue;

            bool attacksZone = false;

            for (const Square& square : zone){
                if (pieceAttacksSquare(board, Square{r, c}, square)){
                    attacksZone = true;break;
                }
            }
            if (attacksZone) totalWeight += attackerWeight(type);
        }
    }
    // Convert total attacking weight into a nonlinear penalty.
    if (totalWeight == 0)
        return 0;
    if (totalWeight <= EvalParams::KING_ATTACK_THRESHOLD_1)
        return EvalParams::KING_ATTACK_PENALTY_1;

    if (totalWeight <= EvalParams::KING_ATTACK_THRESHOLD_2)
        return EvalParams::KING_ATTACK_PENALTY_2;

    if (totalWeight <= EvalParams::KING_ATTACK_THRESHOLD_3)
        return EvalParams::KING_ATTACK_PENALTY_3;

    if (totalWeight <= EvalParams::KING_ATTACK_THRESHOLD_4)
        return EvalParams::KING_ATTACK_PENALTY_4;
    return EvalParams::KING_ATTACK_PENALTY_5;
}


inline int evalKingSafety(const Board& board, int phase)
{
    int whitePenalty =
        pawnShieldPenalty(board, Colour::white) +
        openFilePenalty(board, Colour::white) +
        kingAttackScore(board, Colour::white);

    int blackPenalty =
        pawnShieldPenalty(board, Colour::black) +
        openFilePenalty(board, Colour::black) +
        kingAttackScore(board, Colour::black);

    int scale = 25 + (75 * phase) / EvalParams::MAX_PHASE;

    whitePenalty = (whitePenalty * scale) / 100;
    blackPenalty = (blackPenalty * scale) / 100;

    return blackPenalty - whitePenalty;
}

// Backward-compatible overload for existing tests and callers.
inline int evalKingSafety(const Board& board)
{
    return evalKingSafety(board, gamePhase(board));
}


inline int evalBishopPair(const Board& board)
{
    int whiteBishops = 0;
    int blackBishops = 0;
    for (int r = 0; r < 8; ++r){
        for (int c = 0; c < 8; ++c){
            char piece = board.getPiece(r, c);
            if (piece == 'B') ++whiteBishops;
            else if (piece == 'b') ++blackBishops;
        }
    }
    int score = 0;
    if (whiteBishops >= 2) score += EvalParams::BISHOP_PAIR_BONUS;
    if (blackBishops >= 2) score -= EvalParams::BISHOP_PAIR_BONUS;
    return score;
}

inline bool isOpenFile(const Board& board, int col)
{
    if (col < 0 || col >= 8) return false;
    for (int r = 0; r < 8; ++r){
        char piece = board.getPiece(r, col);
        if (piece == 'P' || piece == 'p') return false;
    }
    return true;
}

inline bool isSemiOpenFile(const Board& board, int col, Colour colour)
{
    if (col < 0 || col >= 8) return false;
    bool hasEnemyPawn = false;
    char friendlyPawn = (colour == Colour::white) ? 'P' : 'p';
    char enemyPawn = (colour == Colour::white) ? 'p' : 'P';
    for (int r = 0; r < 8; ++r) {
        char piece = board.getPiece(r, col);
        if (piece == friendlyPawn) return false;
        if (piece == enemyPawn) hasEnemyPawn = true;
    }
    return hasEnemyPawn;
}

inline int evalRookFiles(const Board& board)
{
    constexpr int OPEN_FILE_BONUS = 25;
    constexpr int SEMI_OPEN_FILE_BONUS = 10;
    int score = 0;
    for (int r = 0; r < 8; ++r){
        for (int c = 0; c < 8; ++c){
            char piece = board.getPiece(r, c);
            if (piece == 'R'){
                if (isOpenFile(board, c)) score += OPEN_FILE_BONUS;
                else if (isSemiOpenFile(board, c, Colour::white)) score += SEMI_OPEN_FILE_BONUS;
            }
            else if (piece == 'r'){
                if (isOpenFile(board, c)) score -= OPEN_FILE_BONUS;
                else if (isSemiOpenFile(board, c, Colour::black))
                    score -= SEMI_OPEN_FILE_BONUS;
            }
        }
    }
    return score;
}

inline int evalRookOnSeventh(const Board& board)
{
    constexpr int ROOK_SEVENTH_RANK_BONUS = 20;
    int score = 0;
    for (int c = 0; c < 8; ++c){
        // White rook on rank 7 (row 1).
        if (board.getPiece(1, c) == 'R') score += ROOK_SEVENTH_RANK_BONUS;
        // Black rook on rank 2 (row 6).
        if (board.getPiece(6, c) == 'r') score -= ROOK_SEVENTH_RANK_BONUS;
    }
    return score;
}

inline int evalActivity(const Board& board)
{
    int score = 0;
    score += evalBishopPair(board);
    score += evalRookFiles(board);
    score += evalRookOnSeventh(board);
    return score;
}


struct EvalTerms
{
    int phase = 0;

    int material = 0;
    int pieceSquare = 0;
    int pawnStructure = 0;
    int mobility = 0;
    int kingSafety = 0;
    int activity = 0;

    // Final score from White's perspective, before tempo.
    int totalWhite = 0;
};

inline EvalTerms getEvalTerms(const Board& board)
{
    EvalTerms terms;

    // Calculate the game phase only once.
    terms.phase = gamePhase(board);

    // Every term uses White's score minus Black's score.
    terms.material = evalMaterial(board);
    terms.pieceSquare = evalPieceSquare(board, terms.phase);
    terms.pawnStructure = evalPawnStructure(board, terms.phase);
    terms.mobility = evalMobility(board);
    terms.kingSafety = evalKingSafety(board, terms.phase);
    terms.activity = evalActivity(board);

    terms.totalWhite =
        terms.material +
        terms.pieceSquare +
        terms.pawnStructure +
        terms.mobility +
        terms.kingSafety +
        terms.activity;

    return terms;
}

// Score from White's perspective, without tempo.
inline int evaluateWhite(const Board& board)
{
    return getEvalTerms(board).totalWhite;
}

// Score from the side-to-move perspective, with a tempo bonus.
inline int evaluate(const Board& board)
{
    const int whiteScore = evaluateWhite(board);

    const int sideToMoveScore =
        (board.getSideToMove() == SideToMove::white)
            ? whiteScore
            : -whiteScore;

    return sideToMoveScore + EvalParams::TEMPO_BONUS;
}

// Print the individual net terms and total score.
inline void evalBreakdown(const Board& board)
{
    const EvalTerms terms = getEvalTerms(board);

    cout << "Game phase: " << terms.phase
         << " / " << EvalParams::MAX_PHASE << '\n';

    cout << "Material:       " << terms.material << '\n';
    cout << "Piece-square:   " << terms.pieceSquare << '\n';
    cout << "Pawn structure: " << terms.pawnStructure << '\n';
    cout << "Mobility:       " << terms.mobility << '\n';
    cout << "King safety:    " << terms.kingSafety << '\n';
    cout << "Activity:       " << terms.activity << '\n';

    cout << "White total:    " << terms.totalWhite << '\n';
    cout << "Evaluation:     " << evaluate(board) << '\n';
}

