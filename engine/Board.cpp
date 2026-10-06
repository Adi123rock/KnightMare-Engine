#include <iostream>
#include <sstream>
#include <climits>
#include "Board.h"

Board::Board(){
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
}
char Board::getPiece(int r,int c) const{
    if(!inBounds(r,c)) return '?';//invalid
    else return position[r][c];
}

void Board::setPiece(int r,int c,char piece){
    if(!inBounds(r,c) || !isValid(piece)) return;
    position[r][c]=piece;
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