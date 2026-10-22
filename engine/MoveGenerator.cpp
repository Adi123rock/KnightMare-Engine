#include "MoveGenerator.h"

namespace
{
    // Knight: eight possible jumps
    constexpr int KNIGHT_DIRECTIONS[8][2] = {
        {-2, -1}, {-2,  1},
        {-1, -2}, {-1,  2},
        { 1, -2}, { 1,  2},
        { 2, -1}, { 2,  1}
    };

    // Bishop: four diagonals
    constexpr int BISHOP_DIRECTIONS[4][2] = {
        {-1, -1}, {-1, 1},
        { 1, -1}, { 1, 1}
    };

    // Rook: four straight directions
    constexpr int ROOK_DIRECTIONS[4][2] = {
        {-1, 0}, {1, 0},
        {0, -1}, {0, 1}
    };

    // King and queen: all eight directions
    constexpr int KING_QUEEN_DIRECTIONS[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},
        { 0, -1},           { 0, 1},
        { 1, -1}, { 1, 0}, { 1, 1}
    };
}

namespace MoveGenerator
{
    bool isEnemy(char piece, Colour colour)
    {
        // Empty and invalid squares are neither friendly nor enemies.
        if (!isValid(piece)) return false;

        if (colour == Colour::white)  return isBlack(piece);

        return isWhite(piece);
    }

    bool isFriendly(char piece, Colour colour)
    {
        if (!isValid(piece)) return false;

        if (colour == Colour::white) return isWhite(piece);

        return isBlack(piece);
    }

    void addMove(vector<Move>& list,const Square& from,const Square& to,
        int flags,char promotion,char captured)
    {
        Move move;

        move.from = from;
        move.to = to;
        move.flags = flags;
        move.promotion = promotion;
        move.captured = captured;

        list.push_back(move);
    }
    void generateKnightMoves(const Board& board,const Square& from,vector<Move>& list)
    {
        // Ensure the source square is on the board.
        if (!inBounds(from.r, from.c)) return;

        char piece = board.getPiece(from.r, from.c);

        // The source must contain a knight.
        if (!isValid(piece) || tolower(piece) != 'n') return;

        // Generate moves only for the side to move.
        Colour side;
        if(board.getSideToMove() == SideToMove::white) side = Colour::white;
        else side = Colour::black;

        if (colourOf(piece) != side) return;

        // Try all 8 knight jumps.
        for (const auto& direction : KNIGHT_DIRECTIONS)
        {
            int targetR = from.r + direction[0];
            int targetC = from.c + direction[1];

            // Ignore targets outside the board.
            if (!inBounds(targetR, targetC)) continue;

            char target = board.getPiece(targetR, targetC);

            // Cannot land on a friendly piece.
            if (isFriendly(target, side)) continue;

            Square to={targetR, targetC};

            // Empty target: normal move.
            if (isEmpty(target))
            {
                addMove(list, from, to);
            }
            // Enemy target: capture.
            else if (isEnemy(target, side))
            {
                addMove(list,from,to,MOVE_CAPTURE,'.',target);
            }
        }
    }

    void generateSlidingMoves(const Board& board,const Square& from,
    const int directions[][2],int directionCount,vector<Move>& list)
    {
        if (!inBounds(from.r, from.c))
            return;

        char piece = board.getPiece(from.r, from.c);

        if (!isValid(piece)) return;

        Colour side =
            board.getSideToMove() == SideToMove::white ? Colour::white: Colour::black;

        if (!isFriendly(piece, side)) return;

        for (int i = 0; i < directionCount; i++)
        {
            int dr = directions[i][0],dc = directions[i][1];

            int targetR = from.r + dr,targetC = from.c + dc;

            while (inBounds(targetR, targetC))
            {
                char target = board.getPiece(targetR, targetC);
                if (isEmpty(target))
                { // Empty square: add the move and keep sliding.
                    addMove(list,from,Square{targetR, targetC});
                }
                else if (isEnemy(target, side))
                { // Enemy: capture it, then stop this direction.
                    addMove(list,from,Square{targetR, targetC},MOVE_CAPTURE,'.',target);
                    break;
                }
                else
                { // Friendly piece: cannot land here or go beyond it.
                    break;
                }
                targetR += dr,targetC += dc;
            }
        }
    }

    void generateBishopMoves(const Board& board,const Square& from,vector<Move>& list)
    {
        if (!inBounds(from.r, from.c)) return;

        char piece = board.getPiece(from.r, from.c);

        if (!isValid(piece) || tolower(piece) != 'b') return;

        Colour side =
            board.getSideToMove() == SideToMove::white? Colour::white: Colour::black;

        if (!isFriendly(piece, side)) return;

        generateSlidingMoves(board,from,BISHOP_DIRECTIONS,4,list);
    }

    void generateRookMoves(const Board& board,const Square& from,vector<Move>& list)
    {
        if (!inBounds(from.r, from.c)) return;

        char piece = board.getPiece(from.r, from.c);

        if (!isValid(piece) || tolower(piece) != 'r') return;

        Colour side =
            board.getSideToMove() == SideToMove::white? Colour::white: Colour::black;

        if (!isFriendly(piece, side)) return;

        generateSlidingMoves(board,from,ROOK_DIRECTIONS,4,list);
    }

    void generateQueenMoves(const Board& board,const Square& from,vector<Move>& list)
    {
        if (!inBounds(from.r, from.c)) return;

        char piece = board.getPiece(from.r, from.c);

        if (!isValid(piece) || tolower(piece) != 'q') return;

        Colour side =
            board.getSideToMove() == SideToMove::white? Colour::white: Colour::black;

        if (!isFriendly(piece, side)) return;

        generateSlidingMoves(board,from,KING_QUEEN_DIRECTIONS,8,list);
    }

    void generateKingMoves(const Board& board, const Square& from, vector<Move>& list)
    {
        if (!inBounds(from.r, from.c)) return;

        char piece = board.getPiece(from.r, from.c);

        if (!isValid(piece) || tolower(piece) != 'k') return;

        Colour side =
            board.getSideToMove() == SideToMove::white? Colour::white : Colour::black;

        if (!isFriendly(piece, side)) return;

        for (const auto& direction : KING_QUEEN_DIRECTIONS)
        {
            int targetR = from.r + direction[0],targetC = from.c + direction[1];

            if (!inBounds(targetR, targetC)) continue;

            char target = board.getPiece(targetR, targetC);

            if (isFriendly(target, side)) continue;

            Square to = {targetR, targetC};

            if (isEmpty(target))
            {
                addMove(list, from, to);
            }
            else if (isEnemy(target, side))
            {
                addMove(list, from, to, MOVE_CAPTURE, '.', target);
            }
        }
    }

    int pawnDirection(Colour colour)
    {
        // White moves toward smaller row numbers.
        return colour == Colour::white ? -1 : 1;
    }

    int pawnStartRow(Colour colour)
    {
        // White starts on row 6; black starts on row 1.
        return colour == Colour::white ? 6 : 1;
    }

    int pawnPromotionRow(Colour colour)
    {
        // White promotes on row 0; black promotes on row 7.
        return colour == Colour::white ? 0 : 7;
    }

    void addPromotionMoves(vector<Move>& list,const Square& from,
        const Square& to,bool isCapture,char captured)
    {
        // A pawn can promote to a queen, rook, bishop, or knight.
        constexpr char PROMOTION_PIECES[] = {'q', 'r', 'b', 'n'};

        for (char promotion : PROMOTION_PIECES)
        {
            int flags = MOVE_PROMOTION;
            // Promotion captures need both flags.
            if (isCapture) flags |= MOVE_CAPTURE;

            addMove(list, from, to, flags, promotion, captured);
        }
    }

    void generateEnPassant(const Board& board,const Square& from,vector<Move>& list)
    {
        char piece = board.getPiece(from.r, from.c);

        // Only pawns can capture en passant.
        if (piece != 'P' && piece != 'p') return;

        Colour side = colourOf(piece);

        Square target = board.getEnPassantSquare();

        if (!inBounds(target.r, target.c)) return;

        int direction = pawnDirection(side);

        // The target must be one row forward and one column sideways.
        if (target.r != from.r + direction || abs(target.c - from.c) != 1) return;

        // The target square must be empty.
        if (!isEmpty(board.getPiece(target.r, target.c))) return;

        // The captured pawn is beside the source square, not on the target.
        char captured = board.getPiece(from.r, target.c);

        // Verify that the captured piece is an enemy pawn.
        if (side == Colour::white && captured != 'p') return;

        if (side == Colour::black && captured != 'P') return;

        addMove(list,from,target,MOVE_CAPTURE | MOVE_EN_PASSANT,EMPTY,captured);
    }


    void generatePawnMoves(const Board& board,const Square& from,vector<Move>& list)
    {
        // Check that the source square is valid.
        if (!inBounds(from.r, from.c)) return;

        char piece = board.getPiece(from.r, from.c);

        // The source must contain a pawn.
        if (!isValid(piece) || tolower(piece) != 'p') return;

        // Generate moves only for the side to move.
        Colour side =
            board.getSideToMove() == SideToMove::white? Colour::white : Colour::black;

        if (!isFriendly(piece, side)) return;

        int direction = pawnDirection(side);
        int startRow = pawnStartRow(side);
        int promotionRow = pawnPromotionRow(side);

        // ---------- Forward moves ----------

        int targetR = from.r + direction;
        int targetC = from.c;

        // A pawn can move forward only if the square is empty.
        if (inBounds(targetR, targetC) &&
            isEmpty(board.getPiece(targetR, targetC)))
        {
            Square to = {targetR, targetC};

            // Reaching the last rank creates four promotion moves.
            if (targetR == promotionRow)
            {
                addPromotionMoves(list, from, to, false, EMPTY);
            }
            else
            {
                addMove(list, from, to);
                // A double push is possible only from the starting row.
                if (from.r == startRow)
                {
                    int doubleR = from.r + 2 * direction;
                    // The intermediate and destination squares must be empty.
                    if (inBounds(doubleR, from.c) && isEmpty(board.getPiece(doubleR, from.c)))
                    {
                        addMove(list, from, Square{doubleR, from.c},MOVE_DOUBLE_PUSH);
                    }
                }
            }
        }

        // ---------- Diagonal captures ----------

        for (int dc : {-1, 1})
        {
            targetR = from.r + direction;
            targetC = from.c + dc;

            // Ignore diagonals outside the board.
            if (!inBounds(targetR, targetC)) continue;

            char target = board.getPiece(targetR, targetC);

            // Pawns capture only enemy pieces diagonally.
            if (!isEnemy(target, side)) continue;

            Square to = {targetR, targetC};

            // Capturing on the last rank also produces four promotions.
            if (targetR == promotionRow)
            {
                addPromotionMoves(list, from, to, true, target);
            }
            else
            {
                addMove(list, from, to, MOVE_CAPTURE, EMPTY, target);
            }
        }
        // ---------- En passant ----------
        generateEnPassant(board, from, list);
    }
//start from here
    bool squaresEmpty(const Board& board, int row, int colFrom, int colTo)
    {
        for (int c = colFrom; c <= colTo; c++)
        {
            if (!isEmpty(board.getPiece(row, c))) return false;
        }
        return true;
    }
    void generateCastlingMoves(const Board& board, vector<Move>& list)
    {
        // Determine which side is moving.
        bool whiteToMove = board.getSideToMove() == SideToMove::white;

        int row = whiteToMove ? 7 : 0;

        char king = whiteToMove ? 'K' : 'k';
        char rook = whiteToMove ? 'R' : 'r';

        // The king must be on its home square.
        if (board.getPiece(row, 4) != king) return;

        Castling rights = board.getCastlingRights();

        // ---------- King-side castling ----------
        bool kingSideRight = whiteToMove ? rights.wk : rights.bk;

        if (kingSideRight && board.getPiece(row, 7) == rook &&
            squaresEmpty(board, row, 5, 6))
        {
            addMove(list,Square{row, 4},Square{row, 6},MOVE_KING_CASTLE);
        }

        // ---------- Queen-side castling ----------
        bool queenSideRight = whiteToMove ? rights.wq : rights.bq;

        if (queenSideRight && board.getPiece(row, 0) == rook 
            && squaresEmpty(board, row, 1, 3))
        {
            addMove(list,Square{row, 4},Square{row, 2},MOVE_QUEEN_CASTLE);
        }
    }

    void generateMovesForPiece(const Board& board,const Square& from,vector<Move>& list)
    {
        if (!inBounds(from.r, from.c)) return;

        char piece = board.getPiece(from.r, from.c);

        if (!isValid(piece)) return;

        // Identify the piece type regardless of its colour.
        char pieceType = tolower(piece);

        switch (pieceType)
        {
            case 'p':
                generatePawnMoves(board, from, list);
                break;

            case 'n':
                generateKnightMoves(board, from, list);
                break;

            case 'b':
                generateBishopMoves(board, from, list);
                break;

            case 'r':
                generateRookMoves(board, from, list);
                break;

            case 'q':
                generateQueenMoves(board, from, list);
                break;

            case 'k':
                generateKingMoves(board, from, list);
                break;
        }
    }
    
    vector<Move> generateAllMoves(const Board& board)
    {
        vector<Move> list;

        Colour side =
            board.getSideToMove() == SideToMove::white? Colour::white: Colour::black;

        // Visit every square on the board.
        for (int r = 0; r < 8; r++)
        {
            for (int c = 0; c < 8; c++)
            {
                char piece = board.getPiece(r, c);
                // Skip empty squares.
                if (!isValid(piece)) continue;

                // Generate moves only for the side to move.
                if (colourOf(piece) != side) continue;

                generateMovesForPiece(board, Square{r, c}, list);
            }
        }
        // Generate castling once for the side to move.
        generateCastlingMoves(board, list);
        return list;
    }
}
