#include "eval.h"
#include "utils.h"
#include "movegen.h"

namespace Eval
{
    static U64 passedMask[2][64];

    inline int mirror_sq(int sq)
    {
        return sq ^ 56;
    }

    static constexpr int gamePhaseInc[12] = {
        0, 0,
        1, 1,
        1, 1,
        2, 2,
        4, 4,
        0, 0};

    static constexpr int MAX_PHASE = 24;

    //-----------------------------------------
    // PeSTO TABLES (VERTICALLY FLIPPED)
    //-----------------------------------------

    const int mg_pawn_table[64] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        -35, -1, -20, -23, -15, 24, 38, -22,
        -26, -4, -4, -10, 3, 3, 33, -12,
        -27, -2, -5, 12, 17, 6, 10, -25,
        -14, 13, 6, 21, 23, 12, 17, -23,
        -6, 7, 26, 31, 65, 56, 25, -20,
        98, 134, 61, 95, 68, 126, 34, -11,
        0, 0, 0, 0, 0, 0, 0, 0
    };

    const int eg_pawn_table[64] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        13, 8, 8, 10, 13, 0, 2, -7,
        4, 7, -6, 1, 0, -5, -1, -8,
        13, 9, -3, -7, -7, -8, 3, -1,
        32, 24, 13, 5, -2, 4, 17, 17,
        94, 100, 85, 67, 56, 53, 82, 84,
        178, 173, 158, 134, 147, 132, 165, 187,
        0, 0, 0, 0, 0, 0, 0, 0
    };

    const int mg_knight_table[64] = {
        -105, -21, -58, -33, -17, -28, -19, -23,
        -29, -53, -12, -3, -1, 18, -14, -19,
        -23, -9, 12, 10, 19, 17, 25, -16,
        -13, 4, 16, 13, 28, 19, 21, -8,
        -9, 17, 19, 53, 37, 69, 18, 22,
        -47, 60, 37, 65, 84, 129, 73, 44,
        -73, -41, 72, 36, 23, 62, 7, -17,
        -167, -89, -34, -49, 61, -97, -15, -107
    };

    const int eg_knight_table[64] = {
        -29, -51, -23, -15, -22, -18, -50, -64,
        -42, -20, -10, -5, -2, -20, -23, -44,
        -23, -3, -1, 15, 10, -3, -20, -22,
        -18, -6, 16, 25, 16, 17, 4, -18,
        -17, 3, 22, 22, 22, 11, 8, -18,
        -24, -20, 10, 9, -1, -9, -19, -41,
        -25, -8, -25, -2, -9, -25, -24, -52,
        -58, -38, -13, -28, -31, -27, -63, -99
    };

    const int mg_bishop_table[64] = {
        -33, -3, -14, -21, -13, -12, -39, -21,
        4, 15, 16, 0, 7, 21, 33, 1,
        0, 15, 15, 15, 14, 27, 18, 10,
        -6, 13, 13, 26, 34, 12, 10, 4,
        -4, 5, 19, 50, 37, 37, 7, -2,
        -16, 37, 43, 40, 35, 50, 37, -2,
        -26, 16, -18, -13, 30, 59, 18, -47,
        -29, 4, -82, -37, -25, -42, 7, -8
    };

    const int eg_bishop_table[64] = {
        -23, -9, -23, -5, -9, -16, -5, -17,
        -14, -18, -7, -1, 4, -9, -15, -27,
        -12, -3, 8, 10, 13, 3, -7, -15,
        -6, 3, 13, 19, 7, 10, -3, -9,
        -3, 9, 12, 9, 14, 10, 3, 2,
        2, -8, 0, -1, -2, 6, 0, 4,
        -8, -4, 7, -12, -3, -13, -4, -14,
        -14, -21, -11, -8, -7, -9, -17, -24
    };

    const int mg_rook_table[64] = {
        -19, -13, 1, 17, 16, 7, -37, -26,
        -44, -16, -20, -9, -1, 11, -6, -71,
        -45, -25, -16, -17, 3, 0, -5, -33,
        -36, -26, -12, -1, 9, -7, 6, -23,
        -24, -11, 7, 26, 24, 35, -8, -20,
        -5, 19, 26, 36, 17, 45, 61, 16,
        27, 32, 58, 62, 80, 67, 26, 44,
        32, 42, 32, 51, 63, 9, 31, 43
    };

    const int eg_rook_table[64] = {
        -9, 2, 3, -1, -5, -13, 4, -20,
        -6, -6, 0, 2, -9, -9, -11, -3,
        -4, 0, -5, -1, -7, -12, -8, -16,
        3, 5, 8, 4, -5, -6, -8, -11,
        4, 3, 13, 1, 2, 1, -1, 2,
        7, 7, 7, 5, 4, -3, -5, -3,
        11, 13, 13, 11, -3, 3, 8, 3,
        13, 10, 18, 15, 12, 12, 8, 5
    };

    const int mg_queen_table[64] = {
        -1, -18, -9, 10, -15, -25, -31, -50,
        -35, -8, 11, 2, 8, 15, -3, 1,
        -14, 2, -11, -2, -5, 2, 14, 5,
        -9, -26, -9, -10, -2, -4, 3, -3,
        -27, -27, -16, -16, -1, 17, -2, 1,
        -13, -17, 7, 8, 29, 56, 47, 57,
        -24, -39, -5, 1, -16, 57, 28, 54,
        -28, 0, 29, 12, 59, 44, 43, 45
    };

    const int eg_queen_table[64] = {
        -33, -28, -22, -43, -5, -32, -20, -41,
        -22, -23, -30, -16, -16, -23, -36, -32,
        -16, -27, 15, 6, 9, 17, 10, 5,
        -18, 28, 19, 47, 31, 34, 39, 23,
        3, 22, 24, 45, 57, 40, 57, 36,
        -20, 6, 9, 49, 47, 35, 19, 9,
        -17, 20, 32, 41, 58, 25, 30, 0,
        -9, 22, 22, 27, 27, 19, 10, 20
    };

    const int mg_king_table[64] = {
        -15, 36, 12, -54, 8, -28, 24, 14,
        1, 7, -8, -64, -43, -16, 9, 8,
        -14, -14, -22, -46, -44, -30, -15, -27,
        -49, -1, -27, -39, -46, -44, -33, -51,
        -17, -20, -12, -27, -30, -25, -14, -36,
        -9, 24, 2, -16, -20, 6, 22, -22,
        29, -1, -20, -7, -8, -4, -38, -29,
        -65, 23, 16, -15, -56, -34, 2, 13
    };

    // NOT PeSTO, manually edited
    const int eg_king_table[64] = {
        -53, -34, -21, -11, -28, -14, -24, -43,
        -27, -11, 4, 13, 14, 4, -5, -17,
        -19, 3, 21, 22, 23, 24, 17, -9,
        -18, 14, 21, 41, 39, 28, 16, -11,
        -8, 24, 34, 40, 41, 33, 26, 3,
        10, 26, 34, 36, 38, 35, 34, 12,
        -12, 17, 14, 17, 17, 28, 23, 11,
        -44, -25, -8, 0, 0, 8, -12, -37
    };

    static const int *mgPestoTable[6] = {
        mg_pawn_table,
        mg_knight_table,
        mg_bishop_table,
        mg_rook_table,
        mg_queen_table,
        mg_king_table};

    static const int *egPestoTable[6] = {
        eg_pawn_table,
        eg_knight_table,
        eg_bishop_table,
        eg_rook_table,
        eg_queen_table,
        eg_king_table};

    int mgTable[12][64];
    int egTable[12][64];

    //-----------------------------------------
    // PAWN STRUCTURE
    //-----------------------------------------

    static constexpr int doubledPawnPenaltyMg = 15;
    static constexpr int doubledPawnPenaltyEg = 10;

    static constexpr int isolatedPawnPenaltyMg = 20;
    static constexpr int isolatedPawnPenaltyEg = 10;

    static constexpr int passedPawnBonusMg[8] = {
        0, 5, 10, 20, 35, 60, 90, 0};

    static constexpr int passedPawnBonusEg[8] = {
        0, 10, 20, 40, 60, 90, 140, 0};

    static constexpr U64 fileMasks[8] = {
        FILE_A, FILE_B, FILE_C, FILE_D,
        FILE_E, FILE_F, FILE_G, FILE_H};

    inline bool has_neighbor_pawns(U64 pawns, int file)
    {
        U64 left = (file > 0) ? fileMasks[file - 1] : 0;
        U64 right = (file < 7) ? fileMasks[file + 1] : 0;
        return pawns & (left | right);
    }

    inline U64 passed_mask_white(int sq)
    {
        int file = sq & 7;
        int rank = sq >> 3;

        U64 mask = 0;

        for (int f = file - 1; f <= file + 1; f++)
        {
            if (f < 0 || f > 7)
                continue;

            for (int r = rank + 1; r < 8; r++)
                mask |= (1ULL << (r * 8 + f));
        }

        return mask;
    }

    inline U64 passed_mask_black(int sq)
    {
        int file = sq & 7;
        int rank = sq >> 3;

        U64 mask = 0;

        for (int f = file - 1; f <= file + 1; f++)
        {
            if (f < 0 || f > 7)
                continue;

            for (int r = 0; r < rank; r++)
                mask |= (1ULL << (r * 8 + f));
        }

        return mask;
    }

    //-----------------------------------------
    // KING SAFETY
    //-----------------------------------------

    static constexpr int kingAttackWeight[6] = {
        0, 1, 1, 2, 4, 0};

    inline int king_safety(const Board &board, bool white)
    {
        int ksq = find_king_sq(board, white ? WHITE : BLACK);

        int penalty = 0;

        int kfile = ksq & 7;
        int krank = ksq >> 3;

        U64 pawns = board.pieces[white ? W_P : B_P];

        int pawnCount = 0;
        int dir = white ? 1 : -1;

        for (int df = -1; df <= 1; df++)
        {
            int f = kfile + df;

            if (f < 0 || f > 7)
                continue;

            for (int r = 1; r <= 2; r++)
            {
                int sq = (krank + r * dir) * 8 + f;

                if (sq < 0 || sq >= 64)
                    continue;

                if (pawns & (1ULL << sq))
                    pawnCount++;
            }
        }

        if (pawnCount == 0)
            penalty += 12;
        else if (pawnCount == 1)
            penalty += 6;
        else if (pawnCount == 2)
            penalty -= 6;
        else
            penalty -= 10;

        U64 enemy = 0;

        for (int p = 0; p < 6; p++)
            enemy |= board.pieces[white ? (p + 6) : p];

        while (enemy)
        {
            int sq = pop_lsb(enemy);

            int dx = (sq & 7) - kfile;
            int dy = (sq >> 3) - krank;

            int dist = abs(dx) + abs(dy);

            if (dist > 3)
                continue;

            int pieceType = -1;

            for (int p = 0; p < 6; p++)
            {
                if (board.pieces[white ? (p + 6) : p] &
                    (1ULL << sq))
                {
                    pieceType = p;
                    break;
                }
            }

            if (pieceType != -1)
                penalty += kingAttackWeight[pieceType];
        }

        return penalty;
    }

    //-----------------------------------------
    // INIT
    //-----------------------------------------

    void init_eval()
    {
        for (int sq = 0; sq < 64; sq++)
        {
            passedMask[WHITE][sq] = passed_mask_white(sq);
            passedMask[BLACK][sq] = passed_mask_black(sq);
        }

        for (int piece = 0; piece < 6; piece++)
        {
            for (int sq = 0; sq < 64; sq++)
            {
                mgTable[2 * piece][sq] =
                    mgValue[piece] +
                    mgPestoTable[piece][sq];

                egTable[2 * piece][sq] =
                    egValue[piece] +
                    egPestoTable[piece][sq];

                mgTable[2 * piece + 1][sq] =
                    mgValue[piece] +
                    mgPestoTable[piece][mirror_sq(sq)];

                egTable[2 * piece + 1][sq] =
                    egValue[piece] +
                    egPestoTable[piece][mirror_sq(sq)];
            }
        }
    }

    //-----------------------------------------
    // MAIN EVAL
    //-----------------------------------------

    int evaluate(const Board &board)
    {
        int mg = 0;
        int eg = 0;
        int phase = 0;

        for (int piece = W_P; piece <= B_K; piece++)
        {
            U64 bb = board.pieces[piece];

            while (bb)
            {
                int sq = pop_lsb(bb);

                int type = piece % 6;
                bool white = piece < 6;

                int tableIndex = 2 * type + (white ? 0 : 1);

                int mgScore = mgTable[tableIndex][sq];
                int egScore = egTable[tableIndex][sq];

                if (white)
                {
                    mg += mgScore;
                    eg += egScore;
                }
                else
                {
                    mg -= mgScore;
                    eg -= egScore;
                }

                phase += gamePhaseInc[piece];
            }
        }

        if (phase > MAX_PHASE)
            phase = MAX_PHASE;

        //---------------------------------
        // PAWNS
        //---------------------------------

        U64 wp = board.pieces[W_P];
        U64 bp = board.pieces[B_P];

        for (int file = 0; file < 8; file++)
        {
            U64 wf = wp & fileMasks[file];
            int wc = bit_count(wf);

            if (wc > 1)
            {
                mg -= (wc - 1) * doubledPawnPenaltyMg;
                eg -= (wc - 1) * doubledPawnPenaltyEg;
            }

            if (wf && !has_neighbor_pawns(wp, file))
            {
                mg -= wc * isolatedPawnPenaltyMg;
                eg -= wc * isolatedPawnPenaltyEg;
            }
        }

        for (int file = 0; file < 8; file++)
        {
            U64 bf = bp & fileMasks[file];
            int bc = bit_count(bf);

            if (bc > 1)
            {
                mg += (bc - 1) * doubledPawnPenaltyMg;
                eg += (bc - 1) * doubledPawnPenaltyEg;
            }

            if (bf && !has_neighbor_pawns(bp, file))
            {
                mg += bc * isolatedPawnPenaltyMg;
                eg += bc * isolatedPawnPenaltyEg;
            }
        }

        //---------------------------------
        // PASSED PAWNS
        //---------------------------------

        U64 whitePawns = wp;

        while (whitePawns)
        {
            int sq = pop_lsb(whitePawns);

            if (!(bp & passedMask[WHITE][sq]))
            {
                int rank = sq >> 3;

                mg += passedPawnBonusMg[rank];
                eg += passedPawnBonusEg[rank];
            }
        }

        U64 blackPawns = bp;

        while (blackPawns)
        {
            int sq = pop_lsb(blackPawns);

            if (!(wp & passedMask[BLACK][sq]))
            {
                int rank = 7 - (sq >> 3);

                mg -= passedPawnBonusMg[rank];
                eg -= passedPawnBonusEg[rank];
            }
        }

        //---------------------------------
        // ROOK OPEN FILES
        //---------------------------------

        static constexpr int rookOpenFileMg     = 20;
        static constexpr int rookSemiOpenFileMg = 10;

        static constexpr int rookOpenFileEg     = 10;
        static constexpr int rookSemiOpenFileEg = 5;

        U64 whiteRooks = board.pieces[W_R];

        while (whiteRooks)
        {
            int sq = pop_lsb(whiteRooks);

            int file = sq & 7;

            bool ownPawn =
                wp & fileMasks[file];

            bool enemyPawn =
                bp & fileMasks[file];

            // fully open
            if (!ownPawn && !enemyPawn)
            {
                mg += rookOpenFileMg;
                eg += rookOpenFileEg;
            }

            // semi-open
            else if (!ownPawn)
            {
                mg += rookSemiOpenFileMg;
                eg += rookSemiOpenFileEg;
            }
        }

        U64 blackRooks = board.pieces[B_R];

        while (blackRooks)
        {
            int sq = pop_lsb(blackRooks);

            int file = sq & 7;

            bool ownPawn =
                bp & fileMasks[file];

            bool enemyPawn =
                wp & fileMasks[file];

            if (!ownPawn && !enemyPawn)
            {
                mg -= rookOpenFileMg;
                eg -= rookOpenFileEg;
            }
            else if (!ownPawn)
            {
                mg -= rookSemiOpenFileMg;
                eg -= rookSemiOpenFileEg;
            }
        }

        //---------------------------------
        // BISHOP PAIR
        //---------------------------------

        if (bit_count(board.pieces[W_B]) >= 2)
        {
            mg += 24;
            eg += 34;
        }

        if (bit_count(board.pieces[B_B]) >= 2)
        {
            mg -= 24;
            eg -= 34;
        }

        //---------------------------------
        // KING SAFETY
        //---------------------------------

        mg -= king_safety(board, true);
        mg += king_safety(board, false);

        //---------------------------------
        // FINAL TAPERED SCORE
        //---------------------------------

        int score =
            (mg * phase +
             eg * (MAX_PHASE - phase)) /
            MAX_PHASE;

        return (board.stm == WHITE)
                   ? score
                   : -score;
    }
}
