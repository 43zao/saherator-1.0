#include "movegen.h"
#include <cstring>
#include <cassert>

// attack tables
U64 pawnMoves[2][64];
U64 pawnAttacks[2][64];
U64 pawnAttackers[2][64];
U64 knightAttacks[64];
U64 kingAttacks[64];

// masks
U64 bishopMasks[64];
U64 rookMasks[64];

// shifts
int bishopShifts[64];
int rookShifts[64];

// Precomputed magic numbers for bishop and rook attacks

U64 bishopMagics[64] = {
    0xFFEDF9FD7CFCFFFFULL, 0xFC0962854A77F576ULL, 0x5822022042000000ULL, 0x2CA804A100200020ULL,
    0x0204042200000900ULL, 0x2002121024000002ULL, 0xFC0A66C64A7EF576ULL, 0x7FFDFDFCBD79FFFFULL,
    0xFC0846A64A34FFF6ULL, 0xFC087A874A3CF7F6ULL, 0x1001080204002100ULL, 0x1810080489021800ULL,
    0x0062040420010A00ULL, 0x5028043004300020ULL, 0xFC0864AE59B4FF76ULL, 0x3C0860AF4B35FF76ULL,
    0x73C01AF56CF4CFFBULL, 0x41A01CFAD64AAFFCULL, 0x040C0422080A0598ULL, 0x4228020082004050ULL,
    0x0200800400E00100ULL, 0x020B001230021040ULL, 0x7C0C028F5B34FF76ULL, 0xFC0A028E5AB4DF76ULL,
    0x0020208050A42180ULL, 0x001004804B280200ULL, 0x2048020024040010ULL, 0x0102C04004010200ULL,
    0x020408204C002010ULL, 0x02411100020080C1ULL, 0x102A008084042100ULL, 0x0941030000A09846ULL,
    0x0244100800400200ULL, 0x4000901010080696ULL, 0x0000280404180020ULL, 0x0800042008240100ULL,
    0x0220008400088020ULL, 0x04020182000904C9ULL, 0x0023010400020600ULL, 0x0041040020110302ULL,
    0xDCEFD9B54BFCC09FULL, 0xF95FFA765AFD602BULL, 0x1401210240484800ULL, 0x0022244208010080ULL,
    0x1105040104000210ULL, 0x2040088800C40081ULL, 0x43FF9A5CF4CA0C01ULL, 0x4BFFCD8E7C587601ULL,
    0xFC0FF2865334F576ULL, 0xFC0BF6CE5924F576ULL, 0x80000B0401040402ULL, 0x0020004821880A00ULL,
    0x8200002022440100ULL, 0x0009431801010068ULL, 0xC3FFB7DC36CA8C89ULL, 0xC3FF8A54F4CA2C89ULL,
    0xFFFFFCFCFD79EDFFULL, 0xFC0863FCCB147576ULL, 0x040C000022013020ULL, 0x2000104000420600ULL,
    0x0400000260142410ULL, 0x0800633408100500ULL, 0xFC087E8E4BB2F736ULL, 0x43FF9E4EF4CA2C89ULL,
};

U64 rookMagics[64] = {
    0x8a80104000800020ULL, 0x140002000100040ULL, 0x2801880a0017001ULL, 0x100081001000420ULL,
    0x200020010080420ULL, 0x3001c0002010008ULL, 0x8480008002000100ULL, 0x2080088004402900ULL,
    0x800098204000ULL, 0x2024401000200040ULL, 0x100802000801000ULL, 0x120800800801000ULL,
    0x208808088000400ULL, 0x2802200800400ULL, 0x2200800100020080ULL, 0x801000060821100ULL,
    0x80044006422000ULL, 0x100808020004000ULL, 0x12108a0010204200ULL, 0x140848010000802ULL,
    0x481828014002800ULL, 0x8094004002004100ULL, 0x4010040010010802ULL, 0x20008806104ULL,
    0x100400080208000ULL, 0x2040002120081000ULL, 0x21200680100081ULL, 0x20100080080080ULL,
    0x2000a00200410ULL, 0x20080800400ULL, 0x80088400100102ULL, 0x80004600042881ULL,
    0x4040008040800020ULL, 0x440003000200801ULL, 0x4200011004500ULL, 0x188020010100100ULL,
    0x14800401802800ULL, 0x2080040080800200ULL, 0x124080204001001ULL, 0x200046502000484ULL,
    0x480400080088020ULL, 0x1000422010034000ULL, 0x30200100110040ULL, 0x100021010009ULL,
    0x2002080100110004ULL, 0x202008004008002ULL, 0x20020004010100ULL, 0x2048440040820001ULL,
    0x101002200408200ULL, 0x40802000401080ULL, 0x4008142004410100ULL, 0x2060820c0120200ULL,
    0x1001004080100ULL, 0x20c020080040080ULL, 0x2935610830022400ULL, 0x44440041009200ULL,
    0x280001040802101ULL, 0x2100190040002085ULL, 0x80c0084100102001ULL, 0x4024081001000421ULL,
    0x20030a0244872ULL, 0x12001008414402ULL, 0x2006104900a0804ULL, 0x1004081002402ULL
};

U64 bishopAttacks[64][512];     // max occupancy for bishop is 512
U64 rookAttacks[64][4096];      // max occupancy for rook is 4096

// helpers

inline static U64 flip_vertical(U64 bb)
{
    bb = ((bb >> 8) & 0x00FF00FF00FF00FFULL) | ((bb & 0x00FF00FF00FF00FFULL) << 8);
    bb = ((bb >> 16) & 0x0000FFFF0000FFFFULL) | ((bb & 0x0000FFFF0000FFFFULL) << 16);
    bb = (bb >> 32) | (bb << 32);
    return bb;
}

static inline U64 bit_at(int sq) { return (sq >= 0 && sq < 64) ? (1ULL << sq) : 0ULL; }

// single-step directional functions expressed as index arithmetic

static inline bool on_board(int sq) { return sq >= 0 && sq < 64; }
static inline int rank_of(int sq) { return sq >> 3; }
static inline int file_of(int sq) { return sq & 7; }

static inline int north_sq(int sq) { return on_board(sq) ? sq + 8 : -1; }
static inline int south_sq(int sq) { return on_board(sq) ? sq - 8 : -1; }
static inline int east_sq(int sq) { return (file_of(sq) < 7) ? sq + 1 : -1; }
static inline int west_sq(int sq) { return (file_of(sq) > 0) ? sq - 1 : -1; }

static inline int north_east_sq(int sq)
{
    int r = rank_of(sq) + 1, f = file_of(sq) + 1;
    return (r <= 7 && f <= 7) ? (sq + 9) : -1;
}
static inline int north_west_sq(int sq)
{
    int r = rank_of(sq) + 1, f = file_of(sq) - 1;
    return (r <= 7 && f >= 0) ? (sq + 7) : -1;
}
static inline int south_east_sq(int sq)
{
    int r = rank_of(sq) - 1, f = file_of(sq) + 1;
    return (r >= 0 && f <= 7) ? (sq - 7) : -1;
}
static inline int south_west_sq(int sq)
{
    int r = rank_of(sq) - 1, f = file_of(sq) - 1;
    return (r >= 0 && f >= 0) ? (sq - 9) : -1;
}

// generate the i-th occupancy variation for a given mask
static U64 set_occupancy(int index, int bits_in_mask, U64 mask)
{
    U64 occupancy = 0ULL;
    U64 temp = mask;
    for (int i = 0; i < bits_in_mask; ++i)
    {
        int square = bit_scan_forward(temp);
        (void)pop_lsb(temp);
        if (index & (1 << i)) set_bit(occupancy, square);
    }
    return occupancy;
}

void init_movegen()
{
    // clear
    memset(pawnMoves, 0, sizeof(pawnMoves));
    memset(pawnAttacks, 0, sizeof(pawnAttacks));
    memset(pawnAttackers, 0, sizeof(pawnAttackers));
    memset(knightAttacks, 0, sizeof(knightAttacks));
    memset(kingAttacks, 0, sizeof(kingAttacks));
    memset(bishopMasks, 0, sizeof(bishopMasks));
    memset(rookMasks, 0, sizeof(rookMasks));
    memset(bishopAttacks, 0, sizeof(bishopAttacks));
    memset(rookAttacks, 0, sizeof(rookAttacks));

    for (int sq = 0; sq < 64; ++sq)
    {
        // pawnAttacks & pawnMoves (static forward masks)
        U64 whiteAtk = 0ULL, whiteAtkrs = 0ULL, whiteMove = 0ULL;

        int ne = north_east_sq(sq);
        int nw = north_west_sq(sq);
        if (ne != -1)
            whiteAtk |= bit_at(ne);
        if (nw != -1)
            whiteAtk |= bit_at(nw);
        pawnAttacks[WHITE][sq] = whiteAtk;

        int se = south_east_sq(sq);
        int sw = south_west_sq(sq);
        if (se != -1)
            whiteAtkrs |= bit_at(se);
        if (sw != -1)
            whiteAtkrs |= bit_at(sw);
        pawnAttackers[WHITE][sq] = whiteAtkrs;

        int one = north_sq(sq);
        if (one != -1)
            whiteMove |= bit_at(one);
        if (rank_of(sq) == 1)
        {
            int two = one != -1 ? north_sq(one) : -1;
            if (two != -1)
                whiteMove |= bit_at(two);
        }
        pawnMoves[WHITE][sq] = whiteMove;

        // black = vertical mirror of white
        int mirror = sq ^ 56;   // flips rank (e.g. a2 -> a7)
        pawnMoves[BLACK][mirror] = flip_vertical(whiteMove);
        pawnAttacks[BLACK][sq] = whiteAtkrs;    // attackers <-> attacks
        pawnAttackers[BLACK][sq] = whiteAtk;

        // knight attacks
        {
            U64 attacks = 0ULL;
            const int deltas[8] = {17, 15, 10, 6, -6, -10, -15, -17};
            for (int d = 0; d < 8; ++d)
            {
                int to = sq + deltas[d];
                if (!on_board(to))
                    continue;
                // exclude wrap-around by checking file/rank diffs
                int fr = rank_of(sq), ff = file_of(sq);
                int tr = rank_of(to), tf = file_of(to);
                if (std::abs(fr - tr) + std::abs(ff - tf) == 3)
                    attacks |= bit_at(to);
            }
            knightAttacks[sq] = attacks;
        }

        // king attacks
        {
            U64 attacks = 0ULL;
            int offsets[8] = {8, -8, 1, -1, 9, 7, -7, -9};
            for (int k = 0; k < 8; ++k)
            {
                int to = sq + offsets[k];
                if (!on_board(to))
                    continue;
                // check wrap-around for east/west moves
                if (std::abs(file_of(to) - file_of(sq)) > 1)
                    continue;
                attacks |= bit_at(to);
            }
            kingAttacks[sq] = attacks;
        }
    }

    // sliding masks
    std::array<std::vector<int>, 64> bMaskSquares{};
    std::array<std::vector<int>, 64> rMaskSquares{};
    for (int sq = 0; sq < 64; ++sq)
    {
        int rank = rank_of(sq);
        int file = file_of(sq);
        U64 bMask = 0ULL, rMask = 0ULL;

        for (int r = rank + 1, f = file + 1; r <= 6 && f <= 6; ++r, ++f)
            set_bit(bMask, r * 8 + f);
        for (int r = rank + 1, f = file - 1; r <= 6 && f >= 1; ++r, --f)
            set_bit(bMask, r * 8 + f);
        for (int r = rank - 1, f = file + 1; r >= 1 && f <= 6; --r, ++f)
            set_bit(bMask, r * 8 + f);
        for (int r = rank - 1, f = file - 1; r >= 1 && f >= 1; --r, --f)
            set_bit(bMask, r * 8 + f);
        bishopMasks[sq] = bMask;

        for (int r = rank + 1; r <= 6; ++r)
            set_bit(rMask, r * 8 + file);
        for (int r = rank - 1; r >= 1; --r)
            set_bit(rMask, r * 8 + file);
        for (int f = file + 1; f <= 6; ++f)
            set_bit(rMask, rank * 8 + f);
        for (int f = file - 1; f >= 1; --f)
            set_bit(rMask, rank * 8 + f);
        rookMasks[sq] = rMask;

        for (int s = 0; s < 64; ++s)
        {
            if (bMask & (1ULL << s))
                bMaskSquares[sq].push_back(s);
            if (rMask & (1ULL << s))
                rMaskSquares[sq].push_back(s);
        }
        bishopShifts[sq] = 64 - (int)bMaskSquares[sq].size();
        rookShifts[sq] = 64 - (int)rMaskSquares[sq].size();
    }

    // compute bishop & rook attack tables
    for (int sq = 0; sq < 64; ++sq)
    {
        int bBits = bit_count(bishopMasks[sq]);
        int bOccCount = (bBits == 0) ? 1 : (1 << bBits);
        for (int index = 0; index < bOccCount; ++index)
        {
            U64 occ = set_occupancy(index, bBits, bishopMasks[sq]);

            int r0 = rank_of(sq), f0 = file_of(sq);
            U64 attacks = 0ULL;
            for (int r = r0 + 1, f = f0 + 1; r <= 7 && f <= 7; ++r, ++f)
            {
                attacks |= bit_at(r * 8 + f);
                if (occ & bit_at(r * 8 + f))
                    break;
            }
            for (int r = r0 + 1, f = f0 - 1; r <= 7 && f >= 0; ++r, --f)
            {
                attacks |= bit_at(r * 8 + f);
                if (occ & bit_at(r * 8 + f))
                    break;
            }
            for (int r = r0 - 1, f = f0 + 1; r >= 0 && f <= 7; --r, ++f)
            {
                attacks |= bit_at(r * 8 + f);
                if (occ & bit_at(r * 8 + f))
                    break;
            }
            for (int r = r0 - 1, f = f0 - 1; r >= 0 && f >= 0; --r, --f)
            {
                attacks |= bit_at(r * 8 + f);
                if (occ & bit_at(r * 8 + f))
                    break;
            }

            U64 magic_index = (occ * bishopMagics[sq]) >> bishopShifts[sq];
            bishopAttacks[sq][magic_index] = attacks;
        }

        int rBits = bit_count(rookMasks[sq]);
        int rOccCount = (rBits == 0) ? 1 : (1 << rBits);
        for (int index = 0; index < rOccCount; ++index)
        {
            U64 occ = set_occupancy(index, rBits, rookMasks[sq]);
            U64 occ_masked = occ & rookMasks[sq];

            int r0 = rank_of(sq), f0 = file_of(sq);
            U64 attacks = 0ULL;
            for (int r = r0 + 1; r <= 7; ++r)
            {
                attacks |= bit_at(r * 8 + f0);
                if (occ & bit_at(r * 8 + f0))
                    break;
            }
            for (int r = r0 - 1; r >= 0; --r)
            {
                attacks |= bit_at(r * 8 + f0);
                if (occ & bit_at(r * 8 + f0))
                    break;
            }
            for (int f = f0 + 1; f <= 7; ++f)
            {
                attacks |= bit_at(r0 * 8 + f);
                if (occ & bit_at(r0 * 8 + f))
                    break;
            }
            for (int f = f0 - 1; f >= 0; --f)
            {
                attacks |= bit_at(r0 * 8 + f);
                if (occ & bit_at(r0 * 8 + f))
                    break;
            }

            U64 magic_index = (occ_masked * rookMagics[sq]) >> rookShifts[sq];
            rookAttacks[sq][(size_t)magic_index] = attacks;
        }
    }
}

bool square_attacked(const Board &board, int sq, Color bySide)
{
    U64 occ = board.occ[BOTH];

    // Pawns
    U64 pawnBB = board.pieces[bySide == WHITE ? W_P : B_P];
    if (get_pawn_attackers(bySide, sq) & pawnBB)
        return true;

    // Knights
    U64 knightBB = board.pieces[bySide == WHITE ? W_N : B_N];
    if (get_knight_attacks(sq) & knightBB)
        return true;

    // Kings
    U64 kingBB = board.pieces[bySide == WHITE ? W_K : B_K];
    if (get_king_attacks(sq) & kingBB)
        return true;

    // Bishops and Queens (diagonals)
    U64 bishopQueenBB = board.pieces[bySide == WHITE ? W_B : B_B] |
                        board.pieces[bySide == WHITE ? W_Q : B_Q];
    if (get_bishop_attacks(sq, occ) & bishopQueenBB)
        return true;

    // Rooks and Queens (orthogonals)
    U64 rookQueenBB = board.pieces[bySide == WHITE ? W_R : B_R] |
                      board.pieces[bySide == WHITE ? W_Q : B_Q];
    if (get_rook_attacks(sq, occ) & rookQueenBB)
        return true;

    return false;
}

void generate_pseudo_legal(const Board &board, MoveList &list)
{
    list.clear();

    Color us = board.stm;
    Color them = opposite(us);

    U64 occ = board.occ[BOTH];
    U64 usOcc = board.occ[us];
    U64 themOcc = board.occ[them];

    auto add_slider_moves = [&](U64 pieces, auto attack_fn, int pieceType)
    {
        while (pieces)
        {
            int sq = pop_lsb(pieces);
            U64 targets = attack_fn(sq, occ) & ~usOcc;

            while (targets)
            {
                int to = pop_lsb(targets);
                int cap = board.piece_at[to];
                list.add(
                    encode_move(
                        sq,
                        to,
                        pieceType,
                        cap,
                        cap != EMPTY ? MF_CAPTURE : 0));
            }
        }
    };

    // ================= PAWNS =================
    U64 pawns = board.pieces[us == WHITE ? W_P : B_P];

    while (pawns)
    {
        int sq = pop_lsb(pawns);

        int forward = (us == WHITE) ? 8 : -8;
        int startRank = (us == WHITE) ? 1 : 6;
        int promoRank = (us == WHITE) ? 7 : 0;

        int one = sq + forward;

        // single push
        if (on_board(one) && !(occ & bit_at(one)))
        {
            if (rank_of(one) == promoRank)
            {
                for (int promo : {W_Q, W_R, W_B, W_N})
                {
                    list.add(
                        encode_move(
                            sq,
                            one,
                            us == WHITE ? promo : promo + 6,
                            EMPTY,
                            MF_PROMO));
                }
            }
            else
            {
                list.add(
                    encode_move(
                        sq,
                        one,
                        us == WHITE ? W_P : B_P,
                        EMPTY,
                        0));

                // double push
                if (rank_of(sq) == startRank)
                {
                    int two = one + forward;
                    if (on_board(two) && !(occ & bit_at(two)))
                    {
                        list.add(
                            encode_move(
                                sq,
                                two,
                                us == WHITE ? W_P : B_P,
                                EMPTY,
                                MF_PAWN2));
                    }
                }
            }
        }

        // captures
        U64 caps = pawnAttacks[us][sq] & themOcc;

        while (caps)
        {
            int to = pop_lsb(caps);
            int capPiece = board.piece_at[to];

            if (rank_of(to) == promoRank)
            {
                for (int promo : {W_Q, W_R, W_B, W_N})
                {
                    list.add(
                        encode_move(
                            sq,
                            to,
                            us == WHITE ? promo : promo + 6,
                            capPiece,
                            MF_PROMO | MF_CAPTURE));
                }
            }
            else
            {
                list.add(
                    encode_move(
                        sq,
                        to,
                        us == WHITE ? W_P : B_P,
                        capPiece,
                        MF_CAPTURE));
            }
        }

        // en passant
        if (board.ep != -1)
        {
            if (pawnAttacks[us][sq] & bit_at(board.ep))
            {
                int capSq = (us == WHITE) ? board.ep - 8 : board.ep + 8;

                if (board.piece_at[capSq] == (us == WHITE ? B_P : W_P))
                {
                    list.add(
                        encode_move(
                            sq,
                            board.ep,
                            us == WHITE ? W_P : B_P,
                            board.piece_at[capSq],
                            MF_EP | MF_CAPTURE));
                }
            }
        }
    }

    // ================= KNIGHTS =================
    U64 knights = board.pieces[us == WHITE ? W_N : B_N];

    while (knights)
    {
        int sq = pop_lsb(knights);
        U64 targets = knightAttacks[sq] & ~usOcc;

        while (targets)
        {
            int to = pop_lsb(targets);
            int cap = board.piece_at[to];

            list.add(
                encode_move(
                    sq,
                    to,
                    us == WHITE ? W_N : B_N,
                    cap,
                    cap != EMPTY ? MF_CAPTURE : 0));
        }
    }

    // ================= BISHOPS =================
    add_slider_moves(
        board.pieces[us == WHITE ? W_B : B_B],
        get_bishop_attacks,
        us == WHITE ? W_B : B_B);

    // ================= ROOKS =================
    add_slider_moves(
        board.pieces[us == WHITE ? W_R : B_R],
        get_rook_attacks,
        us == WHITE ? W_R : B_R);

    // ================= QUEENS =================
    U64 queens = board.pieces[us == WHITE ? W_Q : B_Q];

    while (queens)
    {
        int sq = pop_lsb(queens);

        U64 targets =
            (get_bishop_attacks(sq, occ) |
             get_rook_attacks(sq, occ)) &
            ~usOcc;

        while (targets)
        {
            int to = pop_lsb(targets);
            int cap = board.piece_at[to];

            list.add(
                encode_move(
                    sq,
                    to,
                    us == WHITE ? W_Q : B_Q,
                    cap,
                    cap != EMPTY ? MF_CAPTURE : 0));
        }
    }

    // ================= KING =================
    U64 king = board.pieces[us == WHITE ? W_K : B_K];

    if (king)
    {
        int sq = bit_scan_forward(king);

        U64 targets = kingAttacks[sq] & ~usOcc;

        while (targets)
        {
            int to = pop_lsb(targets);
            int cap = board.piece_at[to];

            list.add(
                encode_move(
                    sq,
                    to,
                    us == WHITE ? W_K : B_K,
                    cap,
                    cap != EMPTY ? MF_CAPTURE : 0));
        }

        // castling
        if (us == WHITE)
        {
            if ((board.castling & CASTLE_WK) &&
                !(occ & (bit_at(5) | bit_at(6))) &&
                !square_attacked(board, 4, BLACK) &&
                !square_attacked(board, 5, BLACK) &&
                !square_attacked(board, 6, BLACK))
            {
                list.add(
                    encode_move(4, 6, W_K, EMPTY, MF_CASTLE));
            }

            if ((board.castling & CASTLE_WQ) &&
                !(occ & (bit_at(1) | bit_at(2) | bit_at(3))) &&
                !square_attacked(board, 4, BLACK) &&
                !square_attacked(board, 3, BLACK) &&
                !square_attacked(board, 2, BLACK))
            {
                list.add(
                    encode_move(4, 2, W_K, EMPTY, MF_CASTLE));
            }
        }
        else
        {
            if ((board.castling & CASTLE_BK) &&
                !(occ & (bit_at(61) | bit_at(62))) &&
                !square_attacked(board, 60, WHITE) &&
                !square_attacked(board, 61, WHITE) &&
                !square_attacked(board, 62, WHITE))
            {
                list.add(
                    encode_move(60, 62, B_K, EMPTY, MF_CASTLE));
            }

            if ((board.castling & CASTLE_BQ) &&
                !(occ & (bit_at(57) | bit_at(58) | bit_at(59))) &&
                !square_attacked(board, 60, WHITE) &&
                !square_attacked(board, 59, WHITE) &&
                !square_attacked(board, 58, WHITE))
            {
                list.add(
                    encode_move(60, 58, B_K, EMPTY, MF_CASTLE));
            }
        }
    }
}

void generate_legal(Board &board, MoveList &list)
{
    generate_pseudo_legal(board, list);
    MoveList legal;

    for (int i = 0; i < list.count; i++)
    {
        Move mv = list.moves[i];
        board.make_move(mv);
        int kingSq = find_king_sq(board, opposite(board.stm));
        if (!square_attacked(board, kingSq, board.stm))
        {
            legal.add(mv);
        }
        board.unmake_move();
    }

    list.count = legal.count;
    memcpy(list.moves, legal.moves, legal.count * sizeof(Move));
}
