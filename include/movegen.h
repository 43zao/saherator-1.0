#pragma once
#include "board.h"
#include <array>

// Contains necessary data structures and does move generation for each piece.
// Enables fast lookup using bitboards and magic bitboards for sliders.

// Using generate_legal(...) is discouraged.
// It makes and unmakes moves to check for legality - unoptimized.
// Use generate_legal_fast(...) from attackers.h instead!

// move representation
using Move = U32;

struct MoveList
{
    static constexpr U8 MAX_MOVES = 255;
    Move moves[MAX_MOVES];
    U8 count = 0;

    inline void add(Move m)
    {
        moves[count++] = m;
    }

    inline void clear()
    {
        count = 0;
    }
};

// attack tables
extern U64 pawnMoves[2][64];
extern U64 pawnAttacks[2][64];
extern U64 pawnAttackers[2][64];
extern U64 knightAttacks[64];
extern U64 kingAttacks[64];

extern U64 bishopMasks[64], rookMasks[64];
extern U64 bishopMagics[64], rookMagics[64];
extern U64 bishopAttacks[64][512];
extern U64 rookAttacks[64][4096];
extern int bishopShifts[64], rookShifts[64];

void init_movegen();

// attack generation helpers

inline U64 get_pawn_moves(Color side, int sq)
{
    return pawnMoves[side][sq];
}

inline U64 get_pawn_attacks(Color side, int sq)
{
    return pawnAttacks[side][sq];
}

inline U64 get_pawn_attackers(Color side, int sq)
{
    return pawnAttackers[side][sq];
}

inline U64 get_knight_attacks(int sq)
{
    return knightAttacks[sq];
}

inline U64 get_king_attacks(int sq)
{
    return kingAttacks[sq];
}

inline U64 get_rook_attacks(int sq, U64 occ)
{
    occ &= rookMasks[sq];
    occ *= rookMagics[sq];
    occ >>= rookShifts[sq];
    return rookAttacks[sq][occ];
}

inline U64 get_bishop_attacks(int sq, U64 occ)
{
    occ &= bishopMasks[sq];
    occ *= bishopMagics[sq];
    occ >>= bishopShifts[sq];
    return bishopAttacks[sq][occ];
}

bool square_attacked(const Board &board, int sq, Color bySide);

inline int find_king_sq(const Board &board, Color side)
{
    return bit_scan_forward(board.pieces[side == WHITE ? W_K : B_K]);
}

// move generation

// generates the pseudo-legal moves for the current position
void generate_pseudo_legal(const Board &board, MoveList &list);

// DEPRECATED: use generate_legal_fast(...)
void generate_legal(Board &board, MoveList &list);
