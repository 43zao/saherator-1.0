#pragma once
#include <cstdint>
#include <cstddef>
#include <random>

// Contains utility structures, definitions and functions.

using U64 = uint64_t;
using U32 = uint32_t;
using U16 = uint16_t;
using U8 = uint8_t;
using S64 = int64_t;
using S32 = int32_t;
using S16 = int16_t;
using S8 = int8_t;

enum Color : int
{
    WHITE = 0,
    BLACK = 1,
    BOTH = 2
};
enum Piece : int
{
    EMPTY = 0xFF,
    W_P = 0, W_N = 1, W_B = 2, W_R = 3, W_Q = 4, W_K = 5,
    B_P = 6, B_N = 7, B_B = 8, B_R = 9, B_Q = 10, B_K = 11
};

constexpr auto STARTFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

constexpr U64 FILE_A = 0x0101010101010101ULL;
constexpr U64 FILE_B = FILE_A << 1;
constexpr U64 FILE_C = FILE_A << 2;
constexpr U64 FILE_D = FILE_A << 3;
constexpr U64 FILE_E = FILE_A << 4;
constexpr U64 FILE_F = FILE_A << 5;
constexpr U64 FILE_G = FILE_A << 6;
constexpr U64 FILE_H = FILE_A << 7;

constexpr U64 RANK_1 = 0x00000000000000FFULL;
constexpr U64 RANK_2 = RANK_1 << (8 * 1);
constexpr U64 RANK_3 = RANK_1 << (8 * 2);
constexpr U64 RANK_4 = RANK_1 << (8 * 3);
constexpr U64 RANK_5 = RANK_1 << (8 * 4);
constexpr U64 RANK_6 = RANK_1 << (8 * 5);
constexpr U64 RANK_7 = RANK_1 << (8 * 6);
constexpr U64 RANK_8 = RANK_1 << (8 * 7);

// castling bits
constexpr U8 CASTLE_WK = 1;
constexpr U8 CASTLE_WQ = 2;
constexpr U8 CASTLE_BK = 4;
constexpr U8 CASTLE_BQ = 8;

// flags bits
constexpr unsigned FLAGS_MASK = 0x1F; // 5 bits
constexpr unsigned PIECE_MASK = 0xF;  // 4 bits for piece index (0..11)
constexpr unsigned SQ_MASK = 0x3F;    // 6 bits for squares (0..63)
constexpr unsigned CAP_NONE = 0xF;    // encoded "no capture" value (4 bits)

// Move flags bitmask (in 20..24)
constexpr U32 MF_CAPTURE = 1 << 0;
constexpr U32 MF_EP = 1 << 1;
constexpr U32 MF_CASTLE = 1 << 2;
constexpr U32 MF_PAWN2 = 1 << 3;
constexpr U32 MF_PROMO = 1 << 4;

// pack move:
// from(0..5) | to(6..11) | piece(12..15) | captured(16..19) | flags(20..24)
// in case of pawn promotion, the piece bits represent the promotion target,
// while the pawn move is implicit.
constexpr inline U32 encode_move(int from, int to, int piece, int captured, U32 flags)
{
    U32 f = (U32)(from & SQ_MASK);
    U32 t = (U32)(to & SQ_MASK) << 6;
    U32 p = (U32)(piece & PIECE_MASK) << 12;
    U32 cap = (U32)(((captured == (int)EMPTY) ? CAP_NONE : (captured & PIECE_MASK)) << 16);
    U32 fl = (U32)((flags & FLAGS_MASK) << 20);
    return f | t | p | cap | fl;
}

constexpr inline int move_from(U32 m) { return (int)(m & SQ_MASK); }
constexpr inline int move_to(U32 m) { return (int)((m >> 6) & SQ_MASK); }
constexpr inline int move_piece(U32 m) { return (int)((m >> 12) & PIECE_MASK); }
constexpr inline int move_captured(U32 m)
{
    int c = (int)((m >> 16) & PIECE_MASK);
    return (c == (int)CAP_NONE) ? EMPTY : c;
}
constexpr inline int move_flags(U32 m) { return (int)((m >> 20) & FLAGS_MASK); }

inline U64 rand64()
{
    static U64 state = 0xDEADBEEFCAFEBABEULL;
    state ^= state >> 12;
    state ^= state << 25;
    state ^= state >> 27;
    return state * 2685821657736338717ULL;
}

inline void set_bit(U64 &bb, int square) { bb |= (1ULL << square); }
inline void clear_bit(U64 &bb, int square) { bb &= ~(1ULL << square); }

[[nodiscard]] inline int bit_scan_forward(U64 bb)
{
#ifdef _MSC_VER
    unsigned long idx;
    _BitScanForward64(&idx, bb);
    return (int)idx;
#else
    return __builtin_ctzll(bb);
#endif
}

[[nodiscard]] inline int pop_lsb(U64 &bb)
{
    int idx = bit_scan_forward(bb);
    bb &= bb - 1;
    return idx;
}

[[nodiscard]] inline int bit_count(U64 bb)
{
#ifdef _MSC_VER
    return (int)__popcnt64(bb);
#else
    return __builtin_popcountll(bb);
#endif
}

[[nodiscard]] inline int bit_scan_reverse(U64 bb)
{
#ifdef _MSC_VER
    unsigned long idx;
    _BitScanReverse64(&idx, bb);
    return (int)idx;
#else
    return 63 ^ __builtin_clzll(bb);
#endif
}
