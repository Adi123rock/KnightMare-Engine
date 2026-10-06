#pragma once
#include <iostream>
using namespace std;

constexpr char EMPTY = '.';

enum class Colour{white,black};
enum class PieceType{
    p,n,b,r,q,k
};
struct Square{
    int r=-1,c=-1;
};

inline bool inBounds(int r, int c){
    return r >= 0 && r <= 7 && c >= 0 && c <= 7;
}

inline bool nameToSquare(const string &name,Square &out){
    if(name.length()!=2) return false;
    if (name[0] < 'a' || name[0] > 'h')
        return false;
    if (name[1] < '1' || name[1] > '8')
        return false;
    out.r=8-(name[1]-'0');
    out.c=name[0]-'a';
    return true;
}

inline string squareToName(const Square &s){
    if(!(inBounds(s.r,s.c))) return "-";
    string name;
    name+=s.c+'a';
    name+=(8-s.r)+'0';
    return name;
}

inline bool isValid(char p){
    p = tolower(p);
    return p == 'p' || p == 'n' || p == 'b' || p == 'r' || p == 'q' || p == 'k';
}


inline bool isBlack(char p){
    if(!isValid(p)) return false;
    return p==tolower(p);
}

inline bool isWhite(char p){
    if(!isValid(p)) return false;
    return p!=tolower(p);
}

inline bool isEmpty(char p){
    return p == EMPTY;//defined above
}