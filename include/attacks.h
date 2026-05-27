#pragma once
#include "movegen.h"
#include <cstring>

// An extension to the standard naive movegen.
// Keeps track of generational state to more efficiently filter for legality.

// returns a bitboard of enemy attackers on given square
U64 attackers_to(const Board &board, int sq, U64 occ, Color side);

// uses attackers to check if side to move is in check at the moment
bool is_in_check(const Board &board);

struct GenState
{
    Color us;
    Color them;
    int kingSq;
    U64 occ;
    U64 usOcc;
    U64 themOcc;
    U64 checkers;       // bitboard of pieces currently giving check
    U64 pinned;         // bb of pinned pieces
    U64 danger;         // bb of squares attacked by enemy pieces
    U64 checkMask;      // mask of squares where a check can be interrupted
    U64 pinRay[64];     // all enemy piece --> our king pin rays
};

// tables used for quick pin scans
extern U64 betweenBB[64][64];
extern U64 lineBB[64][64];

void init_between_tables();

// generates all legal moves in the current position
void generate_legal_fast(const Board &board, MoveList &list);

// generates only the legal captures in the current position
void generate_captures(const Board& board, MoveList& list);
