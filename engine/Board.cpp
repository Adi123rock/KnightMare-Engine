#include <iostream>
#include <sstream>
#include <climits>
#include <array>
#include <cstdint>
#include "Board.h"

namespace {
struct ZobristKeys {
    std::uint64_t piece[12][64]{};
    std::uint64_t side = 0;
    std::uint64_t castling[16]{};
    std::uint64_t enPassant[65]{};
};

std::uint64_t splitmix64(std::uint64_t& state) {
    std::uint64_t z = (state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

const ZobristKeys& zobristKeys() {
    static const ZobristKeys keys = [] {
        ZobristKeys result;
        std::uint64_t seed = 0x4b4e494748544d52ULL;
        for (auto& byPiece : result.piece)
            for (auto& key : byPiece) key = splitmix64(seed);
        result.side = splitmix64(seed);
        for (auto& key : result.castling) key = splitmix64(seed);
        for (auto& key : result.enPassant) key = splitmix64(seed);
        return result;
    }();
    return keys;
}

int pieceIndex(char piece) {
    if (!isValid(piece)) return -1;
    int type = 0;
    switch (static_cast<char>(tolower(static_cast<unsigned char>(piece)))) {
        case 'p': type = 0; break; case 'n': type = 1; break;
        case 'b': type = 2; break; case 'r': type = 3; break;
        case 'q': type = 4; break; default: type = 5; break;
    }
    return type + (isBlack(piece) ? 6 : 0);
}

std::uint64_t pieceSquareKey(char piece, int row, int col) {
    const int index = pieceIndex(piece);
    return index < 0 ? 0 : zobristKeys().piece[index][row * 8 + col];
}

int castlingIndex(const Castling& rights) {
    return (rights.wk ? 1 : 0) | (rights.wq ? 2 : 0) |
           (rights.bk ? 4 : 0) | (rights.bq ? 8 : 0);
}

int enPassantIndex(const Square& square) {
    return inBounds(square.r, square.c) ? square.r * 8 + square.c + 1 : 0;
}
}


Board::Board(){
    history.reserve(128);
    positionKeys.reserve(128);
    hashHistory.reserve(128);
    clear();
}
void Board::clear(){
    sidetomove=SideToMove::white;
    for(int r=0;r<8;r++){
        for(int c=0;c<8;c++) position[r][c]='.';
    }
    halfmove=0,fullmove=1;
    enPassantSquare={-1,-1};
    castling.wk = false;
    castling.wq = false;
    castling.bk = false;
    castling.bq = false;

    history.clear();
    positionKeys.clear();
    hashHistory.clear();
    kingSquares[0] = kingSquares[1] = Square{-1, -1};
    zobristHash = recomputeZobristHash();
}
char Board::getPiece(int r,int c) const{
    if(!inBounds(r,c)) return '?';//invalid
    else return position[r][c];
}


void Board::setPiece(int r, int c, char piece)
{
    if (!inBounds(r, c)) return;

    // Allow empty squares as well as valid chess pieces.
    if (piece != EMPTY && !isValid(piece)) return;
    char previous = position[r][c];
    if (isValid(previous)) {
        zobristHash ^= pieceSquareKey(previous, r, c);
        if (tolower(static_cast<unsigned char>(previous)) == 'k')
            kingSquares[isBlack(previous) ? 1 : 0] = Square{-1, -1};
    }
    position[r][c] = piece;
    if (isValid(piece)) {
        zobristHash ^= pieceSquareKey(piece, r, c);
        if (tolower(static_cast<unsigned char>(piece)) == 'k')
            kingSquares[isBlack(piece) ? 1 : 0] = Square{r, c};
    }
    if (history.empty()) {
        const std::string key = positionKey(*this);
        if (positionKeys.empty()) positionKeys.push_back(key);
        else { positionKeys[0] = key; positionKeys.resize(1); }
        if (hashHistory.empty()) hashHistory.push_back(zobristHash);
        else { hashHistory[0] = zobristHash; hashHistory.resize(1); }
    }
}

SideToMove Board::getSideToMove() const{
    return sidetomove;
}

Castling Board::getCastlingRights() const {
    return castling;
}

Square Board::getEnPassantSquare() const {
    return enPassantSquare;
}

int Board::getHalfMove() const {
    return halfmove;
}

int Board::getFullMove() const {
    return fullmove;
}

int Board::historySize() const
{
    return (int)(history.size());
}

Square Board::getKingSquare(Colour colour) const {
    return kingSquares[colour == Colour::white ? 0 : 1];
}

std::uint64_t Board::getZobristHash() const { return zobristHash; }

const vector<std::uint64_t>& Board::getHashHistory() const { return hashHistory; }

std::uint64_t Board::recomputeZobristHash() const {
    const auto& keys = zobristKeys();
    std::uint64_t hash = 0;
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c)
            hash ^= pieceSquareKey(position[r][c], r, c);
    if (sidetomove == SideToMove::black) hash ^= keys.side;
    hash ^= keys.castling[castlingIndex(castling)];
    hash ^= keys.enPassant[enPassantIndex(enPassantSquare)];
    return hash;
}

const vector<string>& Board::getPositionKeys() const
{
    return positionKeys;
}

void Board::loadStartPosition()
{
    loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

bool parseNonNegativeInt(const std::string& text, int& value)
{
    if(text.empty()) return false;
    long long number = 0;
    for (char ch : text) {
        if (ch<'0' || ch>'9') return false;
        number=number*10+(ch-'0');
        if (number>INT_MAX) return false;
    }
    value=(int)number;
    return true;
}

bool Board::loadFEN(const std::string& fen, std::string* error){
    if(error!=nullptr){
        error->clear();
    }
    //dividing in 6fields
    istringstream iss(fen);

    string fields[6];
    if(!(iss>>fields[0]>>fields[1]>>fields[2]>>fields[3]>>fields[4]>>fields[5])){
        if(error!=nullptr) *error="FEN must contain exactly 6 fields";
        return false;
    }
    string extra;
    if (iss >> extra) {
        if (error != nullptr) *error = "FEN contains more than 6 fields";
        return false;
    }

    //temporary board
    Board temp;
    temp.clear();

    //Board PLacement
    istringstream ranks(fields[0]);
    string rank;
    int r=0;
    while(getline(ranks, rank, '/')){
        if (r >= 8) {
            if (error != nullptr) *error = "FEN has more than 8 ranks";
            return false;
        }
        int c = 0;
        for (char ch : rank) {
            if (ch >= '1' && ch <= '8') c+=ch-'0';
            else if (isValid(ch)) {
                if (c >= 8) {
                    if (error != nullptr) *error = "Too many squares in a rank";
                    return false;
                }
                temp.position[r][c] = ch;
                if (tolower(static_cast<unsigned char>(ch)) == 'k')
                    temp.kingSquares[isBlack(ch) ? 1 : 0] = Square{r, c};
                c++;
            }
            else {
                if (error != nullptr) *error = "Invalid character in board placement";
                return false;
            }
            if (c > 8) {
                if (error != nullptr) *error = "Too many squares in a rank";
                return false;
            }
        }
        if (c != 8) {
            if (error != nullptr) *error = "Rank does not contain exactly 8 squares";
            return false;
        }
        r++;
    }
    // Reject trailing '/'
    if (!fields[0].empty() && fields[0].back() == '/') {
        if (error != nullptr)
            *error = "FEN cannot end with '/'";
        return false;
    }

    if (r != 8) {
        if (error != nullptr)
            *error = "FEN must contain exactly 8 ranks";
        return false;
    }

    //side to move
    if(fields[1]=="w") temp.sidetomove = SideToMove::white;
    else if(fields[1]=="b") temp.sidetomove = SideToMove::black;
    else {
        if(error!=nullptr) *error = "Invalid side to move";
        return false;
    }

    //Castling rights
    temp.castling.wk = false;
    temp.castling.wq = false;
    temp.castling.bk = false;
    temp.castling.bq = false;
    if (fields[2] != "-") {
        for(char ch:fields[2]){
            if(ch=='K'){
                if(temp.castling.wk) {
                    if (error != nullptr) *error = "Duplicate castling right K";
                    return false;
                }
                temp.castling.wk = true;
            }
            else if(ch=='Q'){
                if(temp.castling.wq) {
                    if (error != nullptr) *error = "Duplicate castling right Q";
                    return false;
                }
                temp.castling.wq = true;
            }
            else if(ch=='k'){
                if(temp.castling.bk) {
                    if (error != nullptr) *error = "Duplicate castling right k";
                    return false;
                }
                temp.castling.bk = true;
            }
            else if(ch=='q'){
                if(temp.castling.bq) {
                    if (error != nullptr) *error = "Duplicate castling right q";
                    return false;
                }
                temp.castling.bq = true;
            }
            else {
                if(error!=nullptr) *error="Invalid castling rights";
                return false;
            }
        }
    }

    //enpassant
    temp.enPassantSquare = {-1, -1};
    if (fields[3] != "-") {
        Square sq;
        if (!nameToSquare(fields[3], sq)) {
            if (error != nullptr) *error = "Invalid en passant square";
            return false;
        }
        if (temp.sidetomove == SideToMove::white) {
            if (sq.r != 2) {
                if (error != nullptr) *error = "En passant square must be on rank 6";
                return false;
            }
        }
        else {
            if (sq.r != 5) {
                if (error != nullptr)*error = "En passant square must be on rank 3";
                return false;
            }
        }
        temp.enPassantSquare = sq;
    }

    //Halfmove and fullmove
      //defined parseNonNegativeInt above
    if (!parseNonNegativeInt(fields[4], temp.halfmove)) {
        if (error != nullptr) *error = "Invalid halfmove clock";
        return false;
    }

    if (!parseNonNegativeInt(fields[5], temp.fullmove)) {
        if (error != nullptr) *error = "Invalid fullmove number";
        return false;
    }

    if (temp.fullmove<1) {
        if (error != nullptr) *error = "Fullmove number must be at least 1";
        return false;
    }
    //all succeded so commit
    // All fields parsed successfully.
    // Initialize repetition history with the loaded position.
    temp.positionKeys.clear();
    temp.positionKeys.push_back(positionKey(temp));
    temp.zobristHash = temp.recomputeZobristHash();
    temp.hashHistory.clear();
    temp.hashHistory.push_back(temp.zobristHash);

    // Commit the fully loaded board.
    *this = temp;
    return true;
}


string Board::toFEN() const{
    string fen;

    for(int r=0;r<8;r++){
        int empties=0;
        for(int c=0;c<8;c++){
            char piece=position[r][c];
            if(isEmpty(piece)) empties++;
            else{
                if(empties>0){
                    fen+=char('0'+empties); empties=0;
                }
                fen+=piece;
            }
        }
        if(empties>0)
            fen+=char('0'+empties);
        if(r<7)
            fen+='/';
    }

    fen+=' ';
    if(sidetomove == SideToMove::white) fen += 'w';
    else fen += 'b';

    fen+=' ';
    string castlingRights;

    if(castling.wk) castlingRights+='K';
    if(castling.wq) castlingRights+='Q';
    if(castling.bk) castlingRights+='k';
    if(castling.bq) castlingRights+='q';

    if(castlingRights.empty()) castlingRights='-';
    fen+=castlingRights;

    fen+=' ';
    fen+=(enPassantSquare.r==-1 ? "-" : squareToName(enPassantSquare));

    fen+=' ';
    fen+=to_string(halfmove);

    fen+=' ';
    fen+=to_string(fullmove);
    return fen;
}

string positionKey(const Board& board)
{
    istringstream iss(board.toFEN());

    string placement;
    string side;
    string castlingRights;
    string enPassant;

    iss >> placement >> side >> castlingRights >> enPassant;

    return placement + " " + side + " " +castlingRights + " " + enPassant;
}

void Board::print() const {
    for (int r = 0; r < 8; r++) {
        cout << 8 - r << " ";
        for (int c = 0; c < 8; c++) {
            cout << position[r][c] << ' ';
        }
        cout << '\n';
    }
    cout << "  a b c d e f g h\n";
}

//for makemove()
char Board::promotedPiece(Colour colour, char typeChar) const
{
    if (colour == Colour::white) return (char)(toupper(typeChar));
    return (char)(tolower(typeChar));
}

void Board::applyCastlingRook(const Move& move)
{
    if (hasFlag(move, MOVE_KING_CASTLE))
    {
        // White
        if (move.from.r == 7)
        {
            position[7][5] = position[7][7];
            position[7][7] = EMPTY;
        }
        // Black
        else
        {
            position[0][5] = position[0][7];
            position[0][7] = EMPTY;
        }
    }
    else if (hasFlag(move, MOVE_QUEEN_CASTLE))
    {
        // White
        if (move.from.r == 7)
        {
            position[7][3] = position[7][0];
            position[7][0] = EMPTY;
        }
        // Black
        else
        {
            position[0][3] = position[0][0];
            position[0][0] = EMPTY;
        }
    }
}

void Board::removeEnPassantVictim(const Move& move)
{
    int victimRow = move.from.r;
    int victimCol = move.to.c;

    position[victimRow][victimCol] = EMPTY;
}

void Board::updateEnPassantSquare(const Move& move)
{
    // Every move clears the previous en passant square.
    enPassantSquare = {-1, -1};

    if (!hasFlag(move, MOVE_DOUBLE_PUSH)) return;

    enPassantSquare.r = (move.from.r + move.to.r) / 2;
    enPassantSquare.c = move.from.c;
}

void Board::clearRightForSquare(const Square& square)
{
    // wqs rook corner: a1
    if (square.r == 7 && square.c == 0) castling.wq = false;
    else if (square.r == 7 && square.c == 7) castling.wk = false;// wks rook: h1
    else if (square.r == 0 && square.c == 0) castling.bq = false;// bqs rook: a8
    else if (square.r == 0 && square.c == 7) castling.bk = false;// bks rook: h8
}
void Board::updateCastlingRights(const Move& move, char piece)
{
    // If the king moves, lose both castling rights.
    if (piece == 'K')
    {
        castling.wk = false; castling.wq = false;
    }
    else if (piece == 'k')
    {
        castling.bk = false; castling.bq = false;
    }
    // If a piece leaves a rook corner, clear that corner's right.
    clearRightForSquare(move.from);
    // If a piece arrives on a rook corner, clear that corner's right.
    clearRightForSquare(move.to);
}

void Board::updateClocks(char piece,bool wasCapture)
{
    if (tolower(piece) == 'p' || wasCapture) halfmove = 0;
    else halfmove++;

    if (sidetomove == SideToMove::black) fullmove++;

    if (sidetomove == SideToMove::white) sidetomove = SideToMove::black;
    else sidetomove = SideToMove::white;
}

void Board::makeMove(const Move& move)
{
    // 1. Read the moving piece
    char piece = position[move.from.r][move.from.c];

    // 2. Find the actual captured piece
    char captured = EMPTY;

    if (hasFlag(move, MOVE_EN_PASSANT))
    {
        captured = position[move.from.r][move.to.c];
    }
    else
    {
        captured = position[move.to.r][move.to.c];
    }

    // 3. SAVE UNDO INFORMATION FIRST
    Undo undo;

    undo.move = move;
    undo.captured = captured;
    undo.castling = castling;
    undo.enPassantSquare = enPassantSquare;
    undo.halfmove = halfmove;
    undo.fullmove = fullmove;
    undo.sideToMove = sidetomove;
    undo.zobristHash = zobristHash;

    history.push_back(undo);

    const auto& keys = zobristKeys();
    zobristHash ^= keys.castling[castlingIndex(castling)];
    zobristHash ^= keys.enPassant[enPassantIndex(enPassantSquare)];
    if (sidetomove == SideToMove::black) zobristHash ^= keys.side;
    zobristHash ^= pieceSquareKey(piece, move.from.r, move.from.c);
    if (isValid(captured)) {
        const int captureRow = hasFlag(move, MOVE_EN_PASSANT) ? move.from.r : move.to.r;
        const int captureCol = hasFlag(move, MOVE_EN_PASSANT) ? move.to.c : move.to.c;
        zobristHash ^= pieceSquareKey(captured, captureRow, captureCol);
    }

    // 4. Empty source square
    position[move.from.r][move.from.c] = EMPTY;

    // 5. Determine piece placed on destination
    char pieceToPlace = piece;

    if (hasFlag(move, MOVE_PROMOTION))
    {
        Colour colour = colourOf(piece);
        pieceToPlace = promotedPiece(colour, move.promotion);
    }

    position[move.to.r][move.to.c] = pieceToPlace;
    zobristHash ^= pieceSquareKey(pieceToPlace, move.to.r, move.to.c);
    if (tolower(static_cast<unsigned char>(piece)) == 'k')
        kingSquares[isBlack(piece) ? 1 : 0] = move.to;

    // 6. En passant
    if (hasFlag(move, MOVE_EN_PASSANT))
    {
        removeEnPassantVictim(move);
    }

    // 7. Castling rook
    if (isCastle(move))
    {
        applyCastlingRook(move);
        const int row = move.from.r;
        const int rookFrom = hasFlag(move, MOVE_KING_CASTLE) ? 7 : 0;
        const int rookTo = hasFlag(move, MOVE_KING_CASTLE) ? 5 : 3;
        char rook = position[row][rookTo];
        zobristHash ^= pieceSquareKey(rook, row, rookFrom);
        zobristHash ^= pieceSquareKey(rook, row, rookTo);
    }

    // 8. Update castling rights
    updateCastlingRights(move, piece);


    // 9. Update en passant square
    updateEnPassantSquare(move);

    // 10. Update clocks + side to move
    bool wasCapture = hasFlag(move, MOVE_CAPTURE);
    updateClocks(piece, wasCapture);

    zobristHash ^= keys.castling[castlingIndex(castling)];
    zobristHash ^= keys.enPassant[enPassantIndex(enPassantSquare)];
    if (sidetomove == SideToMove::black) zobristHash ^= keys.side;

    // Record the resulting position after the side has switched.
    positionKeys.push_back(positionKey(*this));
    hashHistory.push_back(zobristHash);
}

void Board::restoreFromUndo(const Undo& record)
{
    sidetomove = record.sideToMove;
    castling = record.castling;
    enPassantSquare = record.enPassantSquare;
    halfmove = record.halfmove;
    fullmove = record.fullmove;
}

void Board::undoCastlingRook(const Move& move)
{
    int row = move.from.r;

    if (hasFlag(move, MOVE_KING_CASTLE))
    {
        // Rook: f-file -> h-file
        position[row][7] = position[row][5];
        position[row][5] = EMPTY;
    }
    else if (hasFlag(move, MOVE_QUEEN_CASTLE))
    {
        // Rook: d-file -> a-file
        position[row][0] = position[row][3];
        position[row][3] = EMPTY;
    }
}

bool Board::undoMove()
{
    // 1. No move to undo
    if (history.empty()) return false;

    // 2. Retrieve and remove the latest undo record
    Undo record = history.back();
    history.pop_back();

    const Move& move = record.move;

    // 3. Restore the previous metadata
    restoreFromUndo(record);

    // 4. Move the piece back
    char piece = position[move.to.r][move.to.c];

    if (hasFlag(move, MOVE_PROMOTION))
    {
        // The original piece was a pawn.
        // sidetomove has already been restored to the mover.
        if (sidetomove == SideToMove::white) piece = 'P';
        else piece = 'p';
    }

    position[move.from.r][move.from.c] = piece;

    // 5. Restore the destination square and captured piece
    position[move.to.r][move.to.c] = EMPTY;

    if (hasFlag(move, MOVE_EN_PASSANT))
    {
        // The captured pawn was beside the destination.
        position[move.from.r][move.to.c] = record.captured;
    }
    else
    {
        // For normal captures, restore the captured piece.
        // If there was no capture, record.captured is EMPTY.
        position[move.to.r][move.to.c] = record.captured;
    }

    // 6. Reverse the rook movement if this was castling
    if (isCastle(move))
    {
        undoCastlingRook(move);
    }

    // Remove the position reached by the move being undone.
    if (!positionKeys.empty())
    {
        positionKeys.pop_back();
    }
    if (!hashHistory.empty()) hashHistory.pop_back();
    zobristHash = record.zobristHash;
    if (tolower(static_cast<unsigned char>(piece)) == 'k')
        kingSquares[sidetomove == SideToMove::white ? 0 : 1] = move.from;
    return true;
}

//updated but not checked
bool Board::isSameAs(const Board& other) const
{
    // 1. Compare all 64 squares.
    for (int r = 0; r < 8; r++)
    {
        for (int c = 0; c < 8; c++)
        {
            if (position[r][c] != other.position[r][c])
                return false;
        }
    }

    // 2. Compare side to move.
    if (sidetomove != other.sidetomove)
        return false;

    // 3. Compare all four castling rights.
    if (castling.wk != other.castling.wk ||
        castling.wq != other.castling.wq ||
        castling.bk != other.castling.bk ||
        castling.bq != other.castling.bq)
    {
        return false;
    }

    // 4. Compare the en passant square.
    if (enPassantSquare.r != other.enPassantSquare.r ||
        enPassantSquare.c != other.enPassantSquare.c)
    {
        return false;
    }

    // 5. Compare halfmove and fullmove clocks.
    if (halfmove != other.halfmove ||
        fullmove != other.fullmove)
    {
        return false;
    }

    // 6. Compare repetition history.
    if (positionKeys != other.positionKeys)
        return false;

    if (zobristHash != other.zobristHash || hashHistory != other.hashHistory)
        return false;

    // 7. Compare undo-history sizes.
    if (history.size() != other.history.size())
        return false;

    return true;
}
