#include <iostream>
#include <string>
#include <ctime>
#include "board.h"
#include "movegen.h"

// This file tests the board.h and movegen.h correctness.
// It has not yet been cleaned up as it was deemed unnecessary.

// print a U64 as 8x8 board
inline static void print_bitboard(U64 bb)
{
    for (int r = 7; r >= 0; --r)
    {
        for (int f = 0; f < 8; ++f)
        {
            int sq = r * 8 + f;
            std::cout << ((bb & (1ULL << sq)) ? "1 " : ". ");
        }
        std::cout << "\n";
    }
}

// board info
inline static void print_board_state(Board &b)
{
    b.print_board();

    std::cout << "\nFEN output: " << b.to_fen() << "\n";

    // occupancies
    std::cout << "\nOccupancies (bitboard view):\n";

    std::cout << "\nWhite occupancy:\n";
    print_bitboard(b.occ[WHITE]);

    std::cout << "\nBlack occupancy:\n";
    print_bitboard(b.occ[BLACK]);

    std::cout << "\nBoth occupancy:\n";
    print_bitboard(b.occ[2]);

    // pieces bitboards
    std::cout << "\nIndividual pieces (bitboard view):\n";
    const char *pieceNames[12] = {
        "W_P", "W_N", "W_B", "W_R", "W_Q", "W_K",
        "B_P", "B_N", "B_B", "B_R", "B_Q", "B_K"};
    for (int i = 0; i < 12; ++i)
    {
        std::cout << pieceNames[i] << ":\n";
        print_bitboard(b.pieces[i]);
        std::cout << "\n";
    }

    // mailbox array
    std::cout << "\nPiece_at squares:\n";
    for (int r = 7; r >= 0; --r)
    {
        for (int f = 0; f < 8; ++f)
        {
            int sq = r * 8 + f;
            int p = b.piece_at[sq];
            if (p == EMPTY)
                std::cout << ". ";
            else if (p <= W_K)
                std::cout << char("PNBRQK"[p]) << " ";
            else
                std::cout << char("pnbrqk"[p - 6]) << " ";
        }
        std::cout << "\n";
    }

    // other properties
    std::cout << "\nSide to move: " << (b.stm == WHITE ? "WHITE" : "BLACK") << "\n";
    std::cout << "Castling rights: "
              << ((b.castling & CASTLE_WK) ? "K" : "")
              << ((b.castling & CASTLE_WQ) ? "Q" : "")
              << ((b.castling & CASTLE_BK) ? "k" : "")
              << ((b.castling & CASTLE_BQ) ? "q" : "")
              << "\n";
    std::cout << "En-passant square: " << (b.ep == -1 ? "-" : std::to_string(b.ep)) << "\n";
    std::cout << "Halfmove clock: " << (int)b.halfmove << "\n";
    std::cout << "Fullmove number: " << b.fullmove << "\n";
    std::cout << "Zobrist: " << b.zobrist << "\n\n";
}

inline static void run_board_tests()
{
    std::cout << "Running board tests...\n\n";
    Board b;
    const char *fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ";
    if (!b.init_from_fen(fen))
    {
        std::cerr << "FEN parse failed!\n";
        return;
    }

    std::cout << "Board after FEN parsing:\n";
    b.print_board();

    std::cout << "\nFEN output: " << b.to_fen() << "\n";

    // occupancies
    std::cout << "\nOccupancies (bitboard view):\n";

    std::cout << "\nWhite occupancy:\n";
    print_bitboard(b.occ[WHITE]);

    std::cout << "\nBlack occupancy:\n";
    print_bitboard(b.occ[BLACK]);

    std::cout << "\nBoth occupancy:\n";
    print_bitboard(b.occ[2]);

    // pieces bitboards
    std::cout << "\nIndividual pieces (bitboard view):\n";
    const char *pieceNames[12] = {
        "W_P", "W_N", "W_B", "W_R", "W_Q", "W_K",
        "B_P", "B_N", "B_B", "B_R", "B_Q", "B_K"};
    for (int i = 0; i < 12; ++i)
    {
        std::cout << pieceNames[i] << ":\n";
        print_bitboard(b.pieces[i]);
        std::cout << "\n";
    }

    // mailbox array
    std::cout << "\nPiece_at squares:\n";
    for (int r = 7; r >= 0; --r)
    {
        for (int f = 0; f < 8; ++f)
        {
            int sq = r * 8 + f;
            int p = b.piece_at[sq];
            if (p == EMPTY)
                std::cout << ". ";
            else if (p <= W_K)
                std::cout << char("PNBRQK"[p]) << " ";
            else
                std::cout << char("pnbrqk"[p - 6]) << " ";
        }
        std::cout << "\n";
    }

    // other properties
    std::cout << "\nSide to move: " << (b.stm == WHITE ? "WHITE" : "BLACK") << "\n";
    std::cout << "Castling rights: "
              << ((b.castling & CASTLE_WK) ? "K" : "")
              << ((b.castling & CASTLE_WQ) ? "Q" : "")
              << ((b.castling & CASTLE_BK) ? "k" : "")
              << ((b.castling & CASTLE_BQ) ? "q" : "")
              << "\n";
    std::cout << "En-passant square: " << (b.ep == -1 ? "-" : std::to_string(b.ep)) << "\n";
    std::cout << "Halfmove clock: " << (int)b.halfmove << "\n";
    std::cout << "Fullmove number: " << b.fullmove << "\n";
}

inline static void run_makemove_tests()
{
    std::cout << "Running makemove tests...\n\n";
    Board b;

    if (!b.init_from_fen(STARTFEN))
    {
        std::cerr << "FEN parse failed!\n";
        return;
    }

    U32 mv1 = encode_move(12, 28, W_P, EMPTY, MF_PAWN2);
    b.make_move(mv1);

    U32 mv2 = encode_move(52, 36, B_P, EMPTY, MF_PAWN2);
    b.make_move(mv2);

    U32 mv3 = encode_move(6, 21, W_N, EMPTY, 0);
    b.make_move(mv3);

    U32 mv4 = encode_move(57, 42, B_N, EMPTY, 0);
    b.make_move(mv4);

    U32 mv5 = encode_move(5, 26, W_B, EMPTY, 0);
    b.make_move(mv5);

    U32 mv6 = encode_move(54, 38, B_P, EMPTY, MF_PAWN2);
    b.make_move(mv6);

    U32 mv7 = encode_move(4, 6, W_K, EMPTY, MF_CASTLE);
    b.make_move(mv7);

    U32 mv8 = encode_move(38, 30, B_P, EMPTY, 0);
    b.make_move(mv8);

    U32 mv9 = encode_move(21, 36, W_N, B_P, 0);
    b.make_move(mv9);

    U32 mv10 = encode_move(48, 40, B_P, EMPTY, 0);
    b.make_move(mv10);

    U32 mv11 = encode_move(15, 31, W_P, EMPTY, MF_PAWN2);
    b.make_move(mv11);

    U32 mv12 = encode_move(30, 23, B_P, EMPTY, MF_EP);
    b.make_move(mv12);

    U32 mv13 = encode_move(36, 42, W_N, B_N, 0);
    b.make_move(mv13);

    U32 mv14 = encode_move(23, 14, B_P, W_P, 0);
    b.make_move(mv14);

    U32 mv15 = encode_move(42, 57, W_N, EMPTY, 0);
    b.make_move(mv15);

    U32 mv16 = encode_move(14, 5, B_Q, W_R, MF_PROMO);
    b.make_move(mv16);

    std::cout << "\n ---------- BOARD AFTER MAKE: ----------\n";
    print_board_state(b);

    b.unmake_move();

    b.unmake_move();

    b.unmake_move();

    b.unmake_move();

    b.unmake_move();

    b.unmake_move();

    b.unmake_move();

    b.unmake_move();

    b.unmake_move();

    b.unmake_move();

    b.unmake_move();

    b.unmake_move();

    b.unmake_move();

    b.unmake_move();

    b.unmake_move();

    b.unmake_move();

    std::cout << "\n ---------- BOARD AFTER UNDO: ----------\n";
    print_board_state(b);
}

inline static void run_movegen_init_tests()
{
    std::cout << "Running movegen init tests...\n\n";
    init_movegen();

    std::cout << "Pawn moves from a7 (BLACK):\n";
    print_bitboard(get_pawn_moves(BLACK, 48));

    std::cout << "Pawn attacks from f5 (WHITE):\n";
    print_bitboard(get_pawn_attacks(WHITE, 37));

    std::cout << "Pawn attackers on f4 (BLACK):\n";
    print_bitboard(get_pawn_attackers(BLACK, 29));

    std::cout << "Knight attacks from b1:\n";
    print_bitboard(get_knight_attacks(1));

    std::cout << "King attacks from e7:\n";
    print_bitboard(get_king_attacks(52));

    std::cout << "Bishop attacks from c1 (empty board):\n";
    print_bitboard(get_bishop_attacks(2, 0ULL));

    std::cout << "Bishop attacks from d5 (empty board):\n";
    print_bitboard(get_bishop_attacks(35, 0ULL));

    std::cout << "Bishop attacks from d5 (2nd and 7th ranks occupied):\n";
    print_bitboard(get_bishop_attacks(35, 0x00FF00000000FF00ULL));

    std::cout << "Rook attacks from f7 (empty board):\n";
    print_bitboard(get_rook_attacks(53, 0ULL));

    std::cout << "Rook attacks from f5 (2nd and 7th ranks occupied):\n";
    print_bitboard(get_rook_attacks(37, 0x00FF00000000FF00ULL));
}

// brute-force ray attacks (full-board occupancy)
inline static U64 brute_bishop_attacks(int sq, U64 occ)
{
    U64 attacks = 0ULL;
    int r0 = sq / 8, f0 = sq % 8;
    for (int r = r0 + 1, f = f0 + 1; r <= 7 && f <= 7; ++r, ++f)
    {
        set_bit(attacks, r * 8 + f);
        if (occ & (1ULL << (r * 8 + f)))
            break;
    }
    for (int r = r0 + 1, f = f0 - 1; r <= 7 && f >= 0; ++r, --f)
    {
        set_bit(attacks, r * 8 + f);
        if (occ & (1ULL << (r * 8 + f)))
            break;
    }
    for (int r = r0 - 1, f = f0 + 1; r >= 0 && f <= 7; --r, ++f)
    {
        set_bit(attacks, r * 8 + f);
        if (occ & (1ULL << (r * 8 + f)))
            break;
    }
    for (int r = r0 - 1, f = f0 - 1; r >= 0 && f >= 0; --r, --f)
    {
        set_bit(attacks, r * 8 + f);
        if (occ & (1ULL << (r * 8 + f)))
            break;
    }
    return attacks;
}

inline static U64 brute_rook_attacks(int sq, U64 occ)
{
    U64 attacks = 0ULL;
    int r0 = sq / 8, f0 = sq % 8;
    for (int r = r0 + 1; r <= 7; ++r)
    {
        set_bit(attacks, r * 8 + f0);
        if (occ & (1ULL << (r * 8 + f0)))
            break;
    }
    for (int r = r0 - 1; r >= 0; --r)
    {
        set_bit(attacks, r * 8 + f0);
        if (occ & (1ULL << (r * 8 + f0)))
            break;
    }
    for (int f = f0 + 1; f <= 7; ++f)
    {
        set_bit(attacks, r0 * 8 + f);
        if (occ & (1ULL << (r0 * 8 + f)))
            break;
    }
    for (int f = f0 - 1; f >= 0; --f)
    {
        set_bit(attacks, r0 * 8 + f);
        if (occ & (1ULL << (r0 * 8 + f)))
            break;
    }
    return attacks;
}

// helper: build occupancy bitboard from mask and index (LSB->first mask-square)
inline static U64 occ_from_index(U64 mask, int index)
{
    U64 occ = 0ULL;
    U64 tmp = mask;
    int i = 0;
    while (tmp)
    {
        int sq = bit_scan_forward(tmp);
        (void)pop_lsb(tmp);
        if (index & (1 << i))
            set_bit(occ, sq);
        ++i;
    }
    return occ;
}

inline static void run_movegen_diagnostics()
{
    init_movegen(); // must be initialized

    std::cout << "Running movegen diagnostics...\n";

    // test bishops
    for (int sq = 0; sq < 64; ++sq)
    {
        U64 mask = bishopMasks[sq];
        int bits = bit_count(mask);
        int occCount = (bits == 0) ? 1 : (1 << bits);

        // test empty occupancy and full-mask occupancy first (quick checks)
        U64 occ_empty = 0ULL;
        U64 occ_full = mask;

        // empty
        {
            U64 tbl = get_bishop_attacks(sq, occ_empty);
            U64 expect = brute_bishop_attacks(sq, occ_empty);
            if (tbl != expect)
            {
                std::cout << "Bishop mismatch (empty) at sq=" << sq << " (rank/file):\n";
                std::cout << " mask bits=" << bits << " shift=" << bishopShifts[sq] << "\n";
                U64 idx = ((occ_empty & mask) * bishopMagics[sq]) >> bishopShifts[sq];
                std::cout << " magic_index=" << idx << " (table entry hex=" << std::hex << bishopAttacks[sq][idx] << std::dec << ")\n";
                std::cout << " expected hex=" << std::hex << expect << std::dec << "\n\n";
                std::cout << "Stored attack:\n";
                print_bitboard(bishopAttacks[sq][idx]);
                std::cout << "Expected attack:\n";
                print_bitboard(expect);
                return;
            }
        }

        // full mask
        {
            U64 tbl = get_bishop_attacks(sq, occ_full);
            U64 expect = brute_bishop_attacks(sq, occ_full);
            if (tbl != expect)
            {
                std::cout << "Bishop mismatch (full-mask) at sq=" << sq << ":\n";
                U64 idx = ((occ_full & mask) * bishopMagics[sq]) >> bishopShifts[sq];
                std::cout << " magic_index=" << idx << " table_hex=" << std::hex << bishopAttacks[sq][idx] << std::dec << "\n";
                std::cout << " expected_hex=" << std::hex << expect << std::dec << "\n\n";
                std::cout << "Stored attack:\n";
                print_bitboard(bishopAttacks[sq][idx]);
                std::cout << "Expected attack:\n";
                print_bitboard(expect);
                return;
            }
        }

        // random a few occupancies mapped through mask (if you want exhaustive, increase loops)
        for (int t = 0; t < 20 && t < occCount; ++t)
        {
            int idx = (t * 997) % occCount; // pseudo-random deterministic index
            U64 occ = occ_from_index(mask, idx);
            U64 tbl = get_bishop_attacks(sq, occ);
            U64 expect = brute_bishop_attacks(sq, occ);
            if (tbl != expect)
            {
                U64 idx_magic = ((occ & mask) * bishopMagics[sq]) >> bishopShifts[sq];
                std::cout << "Bishop mismatch at sq=" << sq << " idx=" << idx << " magic_index=" << idx_magic << "\n";
                std::cout << "mask bits=" << bits << " shift=" << bishopShifts[sq] << "\n";
                std::cout << "Stored attack:\n";
                print_bitboard(tbl);
                std::cout << "Expected attack:\n";
                print_bitboard(expect);
                return;
            }
        }
    }

    // test rooks (same pattern)
    for (int sq = 0; sq < 64; ++sq)
    {
        U64 mask = rookMasks[sq];
        int bits = bit_count(mask);
        int occCount = (bits == 0) ? 1 : (1 << bits);

        U64 occ_empty = 0ULL;
        U64 occ_full = mask;

        {
            U64 tbl = get_rook_attacks(sq, occ_empty);
            U64 expect = brute_rook_attacks(sq, occ_empty);
            if (tbl != expect)
            {
                std::cout << "Rook mismatch (empty) at sq=" << sq << ":\n";
                U64 idx = ((occ_empty & mask) * rookMagics[sq]) >> rookShifts[sq];
                std::cout << " magic_index=" << idx << " table_hex=" << std::hex << rookAttacks[sq][idx] << std::dec << "\n";
                std::cout << " expected_hex=" << std::hex << expect << std::dec << "\n\n";
                std::cout << "Stored attack:\n";
                print_bitboard(rookAttacks[sq][idx]);
                std::cout << "Expected attack:\n";
                print_bitboard(expect);
                return;
            }
        }
        {
            U64 tbl = get_rook_attacks(sq, occ_full);
            U64 expect = brute_rook_attacks(sq, occ_full);
            if (tbl != expect)
            {
                std::cout << "Rook mismatch (full-mask) at sq=" << sq << ":\n";
                U64 idx = ((occ_full & mask) * rookMagics[sq]) >> rookShifts[sq];
                std::cout << " magic_index=" << idx << " table_hex=" << std::hex << rookAttacks[sq][idx] << std::dec << "\n";
                std::cout << " expected_hex=" << std::hex << expect << std::dec << "\n\n";
                std::cout << "Stored attack:\n";
                print_bitboard(rookAttacks[sq][idx]);
                std::cout << "Expected attack:\n";
                print_bitboard(expect);
                return;
            }
        }

        for (int t = 0; t < 20 && t < occCount; ++t)
        {
            int idx = (t * 991) % occCount;
            U64 occ = occ_from_index(mask, idx);
            U64 tbl = get_rook_attacks(sq, occ);
            U64 expect = brute_rook_attacks(sq, occ);
            if (tbl != expect)
            {
                U64 idx_magic = ((occ & mask) * rookMagics[sq]) >> rookShifts[sq];
                std::cout << "Rook mismatch at sq=" << sq << " idx=" << idx << " magic_index=" << idx_magic << "\n";
                std::cout << "mask bits=" << bits << " shift=" << rookShifts[sq] << "\n";
                std::cout << "Stored attack:\n";
                print_bitboard(tbl);
                std::cout << "Expected attack:\n";
                print_bitboard(expect);
                return;
            }
        }
    }

    std::cout << "All bishop/rook quick-tests passed (empty/full/random samples).\n";
}

inline static void test_random_bishops()
{
    init_movegen();

    std::srand((unsigned)std::time(nullptr));

    std::cout << "Random bishop masks, shifts, and attacks:\n\n";

    for (int i = 0; i < 3; ++i)
    {
        int sq = std::rand() % 64;

        std::cout << "Square: " << sq << "\n";

        std::cout << "Mask:\n";
        print_bitboard(bishopMasks[sq]);
        std::cout << "Shift: " << bishopShifts[sq] << "\n";

        // pick a random occupancy index for this bishop
        int bBits = bit_count(bishopMasks[sq]);
        int occIndex = (bBits == 0) ? 0 : (std::rand() % (1 << bBits));

        std::cout << "Attack for random occupancy index " << occIndex << ":\n";
        print_bitboard(bishopAttacks[sq][occIndex]);
        std::cout << "------------------------\n";
    }
}

inline static void test_attacked_sq()
{
    init_movegen();

    // Seed randomness
    std::srand((unsigned)std::time(nullptr));

    // Example FEN: random middlegame-like position
    const char *fen = "r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 2 4";

    Board board;
    board.init_from_fen(fen);

    std::cout << "Testing position: " << fen << "\n";
    board.print_board();

    // Pick a random square to test
    int sq = std::rand() % 64;
    int rank = sq / 8, file = sq % 8;
    char fileChar = 'a' + file;
    char rankChar = '1' + rank;

    std::cout << "Testing square: " << fileChar << rankChar << " (" << sq << ")\n";

    bool attackedByWhite = square_attacked(board, sq, WHITE);
    bool attackedByBlack = square_attacked(board, sq, BLACK);

    std::cout << "Attacked by White? " << (attackedByWhite ? "yes" : "no") << "\n";
    std::cout << "Attacked by Black? " << (attackedByBlack ? "yes" : "no") << "\n";
}
