#pragma once
#include <string>
#include "utils.h"

// Represents the chessboard and its state over time.
// Contains relevant methods for board updating and manipulation.

struct Undo
{
    U64 zobrist;        // full Zobrist hash before move
    U32 move;           // the move made
    U8 moved_piece;     // piece that was on "from" before the move
    U8 captured;        // captured piece
    U8 castling;        // castling rights before move
    S8 ep;              // en-passant square before move
    U8 halfmove;        // halfmove clock before move
};

struct Board
{
    U64 pieces[12];     // bitboards for each type of piece
    U64 occ[3];         // white, black, both
    U64 zobrist;
    Color stm;          // side to move
    U16 fullmove;       
    U8 piece_at[64];    // mailbox: index 0..11 or Piece::EMPTY
    U8 castling;        // bits
    S8 ep;              // en-passant square or -1
    U8 halfmove;

    // maximum reachable ply in one game - 512 moves (low estimate)
    static constexpr U16 MAX_PLY = 1024;
    Undo undo_stack[MAX_PLY];
    U16 ply = 0;

    Board();
    bool init_from_fen(const char *fen);
    std::string to_fen() const;
    void print_board() const;
    void reset();
    inline void update_occupancies();
    bool make_move(U32 mv);
    void unmake_move();
    bool make_null_move();
    void unmake_null_move();
};

// zobrist keys
extern U64 zobPiece[12][64];
extern U64 zobEp[64];
extern U64 zobCastling[16];
extern U64 zobSide;

inline Color opposite(Color c) { return c == WHITE ? BLACK : WHITE; }
inline bool square_occupied(const Board &b, int sq) { return b.piece_at[sq] != EMPTY; }
