#include "attacks.h"

U64 betweenBB[64][64];
U64 lineBB[64][64];

// returns a bitboard of enemy attackers on given square
U64 attackers_to(const Board &board, int sq, U64 occ, Color side)
{
    U64 attackers = 0ULL;

    if (side == WHITE)
    {
        attackers |= get_pawn_attackers(WHITE, sq) & board.pieces[W_P];

        attackers |= get_knight_attacks(sq) & board.pieces[W_N];

        attackers |= get_king_attacks(sq) & board.pieces[W_K];

        attackers |= get_bishop_attacks(sq, occ) & (board.pieces[W_B] | board.pieces[W_Q]);

        attackers |= get_rook_attacks(sq, occ) & (board.pieces[W_R] | board.pieces[W_Q]);
    }
    else
    {
        attackers |= get_pawn_attackers(BLACK, sq) & board.pieces[B_P];

        attackers |= get_knight_attacks(sq) & board.pieces[B_N];

        attackers |= get_king_attacks(sq) & board.pieces[B_K];

        attackers |= get_bishop_attacks(sq, occ) & (board.pieces[B_B] | board.pieces[B_Q]);

        attackers |= get_rook_attacks(sq, occ) & (board.pieces[B_R] | board.pieces[B_Q]);
    }

    return attackers;
}

// uses attackers to check if side to move is in check at the moment
bool is_in_check(const Board &board)
{
    return attackers_to(
               board,
               find_king_sq(board, board.stm),
               board.occ[BOTH],
               opposite(board.stm)) != 0ULL;
}

static inline bool aligned(int a, int b)
{
    int ra = a / 8;
    int fa = a % 8;

    int rb = b / 8;
    int fb = b % 8;

    return ra == rb ||
           fa == fb ||
           abs(ra - rb) == abs(fa - fb);
}

void init_between_tables()
{
    memset(betweenBB, 0, sizeof(betweenBB));
    memset(lineBB, 0, sizeof(lineBB));

    for (int a = 0; a < 64; ++a)
    {
        for (int b = 0; b < 64; ++b)
        {
            if (a == b) continue;

            if (!aligned(a, b)) continue;

            int ra = a / 8;
            int fa = a % 8;

            int rb = b / 8;
            int fb = b % 8;

            int dr = (rb > ra) - (rb < ra);
            int df = (fb > fa) - (fb < fa);

            int r = ra + dr;
            int f = fa + df;

            U64 between = 0ULL;
            U64 line = (1ULL << a) | (1ULL << b);

            while (r != rb || f != fb)
            {
                int sq = r * 8 + f;

                between |= (1ULL << sq);
                line |= (1ULL << sq);

                r += dr;
                f += df;
            }

            between &= ~(1ULL << a);
            between &= ~(1ULL << b);

            lineBB[a][b] = line;
            betweenBB[a][b] = between;
        }
    }
}

// computes the bitboard of squares attacked by the enemy pieces
static U64 compute_danger(const Board &board, Color enemy, int ourKingSq)
{
    U64 danger = 0ULL;

    U64 occ = board.occ[BOTH];
    occ &= ~(1ULL << ourKingSq);

    U64 pawns = board.pieces[enemy == WHITE ? W_P : B_P];

    while (pawns)
    {
        int sq = pop_lsb(pawns);
        danger |= pawnAttacks[enemy][sq];
    }

    U64 knights = board.pieces[enemy == WHITE ? W_N : B_N];

    while (knights)
    {
        int sq = pop_lsb(knights);
        danger |= knightAttacks[sq];
    }

    U64 bishops =
        board.pieces[enemy == WHITE ? W_B : B_B] |
        board.pieces[enemy == WHITE ? W_Q : B_Q];

    while (bishops)
    {
        int sq = pop_lsb(bishops);
        danger |= get_bishop_attacks(sq, occ);
    }

    U64 rooks =
        board.pieces[enemy == WHITE ? W_R : B_R] |
        board.pieces[enemy == WHITE ? W_Q : B_Q];

    while (rooks)
    {
        int sq = pop_lsb(rooks);
        danger |= get_rook_attacks(sq, occ);
    }

    int enemyKingSq = find_king_sq(board, enemy);

    danger |= kingAttacks[enemyKingSq];

    return danger;
}

// computes pined pieces and rays for GenState
static void compute_pins(const Board &board, GenState &st)
{
    st.pinned = 0ULL;

    for (int i = 0; i < 64; ++i)
        st.pinRay[i] = ~0ULL;

    const int directions[8] = { 8, -8, 1, -1, 9, -9, 7, -7 };

    for (int d = 0; d < 8; ++d)
    {
        int dir = directions[d];

        int sq = st.kingSq;

        int candidate = -1;

        while (true)
        {
            int next = sq + dir;

            if (next < 0 || next >= 64)
                break;

            if (abs((next % 8) - (sq % 8)) > 1 &&
                (dir == 1 || dir == -1 ||
                 dir == 9 || dir == -9 ||
                 dir == 7 || dir == -7))
                break;

            sq = next;

            U64 bit = (1ULL << sq);

            if (!(st.occ & bit)) continue;

            if (bit & st.usOcc)
            {
                if (candidate == -1)
                {
                    candidate = sq;
                }
                else
                {
                    break;
                }
            }
            else
            {
                int piece = board.piece_at[sq];

                bool rookLike =
                    dir == 8 || dir == -8 ||
                    dir == 1 || dir == -1;

                bool bishopLike =
                    dir == 9 || dir == -9 ||
                    dir == 7 || dir == -7;

                bool pinner = false;

                if (rookLike)
                {
                    pinner =
                        piece == (st.them == WHITE ? W_R : B_R) ||
                        piece == (st.them == WHITE ? W_Q : B_Q);
                }

                if (bishopLike)
                {
                    pinner = pinner ||
                             piece == (st.them == WHITE ? W_B : B_B) ||
                             piece == (st.them == WHITE ? W_Q : B_Q);
                }

                if (candidate != -1 && pinner)
                {
                    st.pinned |= (1ULL << candidate);

                    st.pinRay[candidate] =
                        lineBB[st.kingSq][sq];
                }

                break;
            }
        }
    }
}

// computes the mask for interrupting a check
static void compute_check_mask(const Board &board, GenState &st)
{
    int count = bit_count(st.checkers);

    if (count == 0)
    {
        st.checkMask = ~0ULL;
        return;
    }

    if (count >= 2)
    {
        st.checkMask = 0ULL;
        return;
    }

    int checkerSq = bit_scan_forward(st.checkers);

    int piece = board.piece_at[checkerSq];

    bool slider =
        piece == W_B || piece == B_B ||
        piece == W_R || piece == B_R ||
        piece == W_Q || piece == B_Q;

    if (!slider)
    {
        st.checkMask = (1ULL << checkerSq);
    }
    else
    {
        st.checkMask =
            betweenBB[st.kingSq][checkerSq] | (1ULL << checkerSq);
    }
}

// computes the full GenState for a board using helper functions
static GenState compute_state(const Board &board)
{
    GenState st{};

    st.us = board.stm;
    st.them = opposite(st.us);

    st.occ = board.occ[BOTH];

    st.usOcc = board.occ[st.us];
    st.themOcc = board.occ[st.them];

    st.kingSq = find_king_sq(board, st.us);

    st.checkers = attackers_to(
        board,
        st.kingSq,
        st.occ,
        st.them);

    st.danger = compute_danger(
        board,
        st.them,
        st.kingSq);

    compute_pins(board, st);

    compute_check_mask(board, st);

    return st;
}

// used for non-king pieces: checks legality using check / pin masks
static inline bool move_allowed(const GenState &st, int from, int to)
{
    U64 toBB = (1ULL << to);

    if (!(toBB & st.checkMask))
        return false;

    if (st.pinned & (1ULL << from))
    {
        if (!(toBB & st.pinRay[from]))
            return false;
    }

    return true;
}

static void generate_king_moves(const Board &board, const GenState &st, MoveList &list)
{
    int sq = st.kingSq;

    U64 moves = kingAttacks[sq] & ~st.usOcc & ~st.danger;

    while (moves)
    {
        int to = pop_lsb(moves);

        int cap = board.piece_at[to];

        list.add(
            encode_move(
                sq,
                to,
                st.us == WHITE ? W_K : B_K,
                cap,
                cap != EMPTY ? MF_CAPTURE : 0));
    }

    // castling
    if (st.checkers) return;

    if (st.us == WHITE)
    {
        // kingside
        if ((board.castling & CASTLE_WK) &&
            !(st.occ & ((1ULL << 5) | (1ULL << 6))) &&
            !(st.danger & ((1ULL << 5) | (1ULL << 6))))
        {
            list.add(
                encode_move(
                    4,
                    6,
                    W_K,
                    EMPTY,
                    MF_CASTLE));
        }

        // queenside
        if ((board.castling & CASTLE_WQ) &&
            !(st.occ & ((1ULL << 1) |
                        (1ULL << 2) |
                        (1ULL << 3))) &&
            !(st.danger & ((1ULL << 2) |
                           (1ULL << 3))))
        {
            list.add(
                encode_move(
                    4,
                    2,
                    W_K,
                    EMPTY,
                    MF_CASTLE));
        }
    }
    else
    {
        // kingside
        if ((board.castling & CASTLE_BK) &&
            !(st.occ & ((1ULL << 61) | (1ULL << 62))) &&
            !(st.danger & ((1ULL << 61) | (1ULL << 62))))
        {
            list.add(
                encode_move(
                    60,
                    62,
                    B_K,
                    EMPTY,
                    MF_CASTLE));
        }

        // queenside
        if ((board.castling & CASTLE_BQ) &&
            !(st.occ & ((1ULL << 57) |
                        (1ULL << 58) |
                        (1ULL << 59))) &&
            !(st.danger & ((1ULL << 58) |
                           (1ULL << 59))))
        {
            list.add(
                encode_move(
                    60,
                    58,
                    B_K,
                    EMPTY,
                    MF_CASTLE));
        }
    }
}

// special case - en passant legality
static inline bool ep_legal(const Board &board, const GenState &st, int from, int to)
{
    int capSq = (st.us == WHITE) ? to - 8 : to + 8;

    U64 occ = st.occ;

    // remove moving pawn
    occ &= ~(1ULL << from);

    // remove captured pawn
    occ &= ~(1ULL << capSq);

    // place pawn on destination
    occ |= (1ULL << to);

    int kingSq = st.kingSq;

    // Pawns
    U64 pawnBB = board.pieces[st.them == WHITE ? W_P : B_P];
    pawnBB &= ~(1ULL << capSq);
    if (get_pawn_attackers(st.them, kingSq) & pawnBB)
        return false;

    /*
    // Knights
    U64 knightBB = board.pieces[st.them == WHITE ? W_N : B_N];
    if (get_knight_attacks(kingSq) & knightBB)
        return false;

    // Kings
    U64 kingBB = board.pieces[st.them == WHITE ? W_K : B_K];
    if (get_king_attacks(kingSq) & kingBB)
        return false;
    */

    // Bishops and Queens (diagonals)
    U64 bishopQueenBB = board.pieces[st.them == WHITE ? W_B : B_B] |
                        board.pieces[st.them == WHITE ? W_Q : B_Q];
    if (get_bishop_attacks(kingSq, occ) & bishopQueenBB)
        return false;

    // Rooks and Queens (orthogonals)
    U64 rookQueenBB = board.pieces[st.them == WHITE ? W_R : B_R] |
                      board.pieces[st.them == WHITE ? W_Q : B_Q];
    if (get_rook_attacks(kingSq, occ) & rookQueenBB)
        return false;

    return true;
}

// generates all legal moves in the current position
// fast due to not having to make actual moves on the board
void generate_legal_fast(const Board &board, MoveList &list)
{
    list.clear();

    GenState st = compute_state(board);

    // king moves are always generated separately
    generate_king_moves(board, st, list);

    // double check -> only king moves legal
    if (bit_count(st.checkers) >= 2) return;

    const int pawn =
        st.us == WHITE ? W_P : B_P;

    const int knight =
        st.us == WHITE ? W_N : B_N;

    const int bishop =
        st.us == WHITE ? W_B : B_B;

    const int rook =
        st.us == WHITE ? W_R : B_R;

    const int queen =
        st.us == WHITE ? W_Q : B_Q;

    // =========================================
    // PAWNS
    // =========================================

    U64 pawns = board.pieces[pawn];

    while (pawns)
    {
        int from = pop_lsb(pawns);

        int forward =
            st.us == WHITE ? 8 : -8;

        int startRank =
            st.us == WHITE ? 1 : 6;

        int promoRank =
            st.us == WHITE ? 7 : 0;

        // single push
        int to = from + forward;

        if (to >= 0 &&
            to < 64 &&
            board.piece_at[to] == EMPTY)
        {
            if (move_allowed(st, from, to))
            {
                if ((to / 8) == promoRank)
                {
                    int promos[4] =
                        {
                            queen,
                            rook,
                            bishop,
                            knight};

                    for (int p = 0; p < 4; ++p)
                    {
                        list.add(
                            encode_move(
                                from,
                                to,
                                promos[p],
                                EMPTY,
                                MF_PROMO));
                    }
                }
                else
                {
                    list.add(
                        encode_move(
                            from,
                            to,
                            pawn,
                            EMPTY,
                            0));
                }
            }
        }

        // double push
        if ((from / 8) == startRank)
        {
            int to2 = to + forward;

            if (to2 >= 0 &&
                to2 < 64 &&
                board.piece_at[to] == EMPTY &&
                board.piece_at[to2] == EMPTY)
            {
                if (move_allowed(st, from, to2))
                {
                    list.add(
                        encode_move(
                            from,
                            to2,
                            pawn,
                            EMPTY,
                            MF_PAWN2));
                }
            }
        }

        // captures
        U64 caps =
            pawnAttacks[st.us][from] & st.themOcc;

        while (caps)
        {
            to = pop_lsb(caps);

            if (!move_allowed(st, from, to))
                continue;

            int cap =
                board.piece_at[to];

            if ((to / 8) == promoRank)
            {
                int promos[4] =
                    {
                        queen,
                        rook,
                        bishop,
                        knight};

                for (int p = 0; p < 4; ++p)
                {
                    list.add(
                        encode_move(
                            from,
                            to,
                            promos[p],
                            cap,
                            MF_PROMO | MF_CAPTURE));
                }
            }
            else
            {
                list.add(
                    encode_move(
                        from,
                        to,
                        pawn,
                        cap,
                        MF_CAPTURE));
            }
        }

        // en passant
        if (board.ep != -1)
        {
            U64 epTargets =
                pawnAttacks[st.us][from] & (1ULL << board.ep);

            if (epTargets)
            {
                int epTo = board.ep;

                if (ep_legal(board, st, from, epTo))
                {
                    int capPawn =
                        st.us == WHITE ? B_P : W_P;

                    list.add(
                        encode_move(
                            from,
                            epTo,
                            pawn,
                            capPawn,
                            MF_EP | MF_CAPTURE));
                }
            }
        }
    }

    // =========================================
    // KNIGHTS
    // =========================================

    U64 knights = board.pieces[knight];

    while (knights)
    {
        int from = pop_lsb(knights);

        // pinned knight cannot move
        if (st.pinned & (1ULL << from))
            continue;

        U64 moves =
            knightAttacks[from] & ~st.usOcc & st.checkMask;

        while (moves)
        {
            int to = pop_lsb(moves);

            int cap =
                board.piece_at[to];

            list.add(
                encode_move(
                    from,
                    to,
                    knight,
                    cap,
                    cap != EMPTY
                        ? MF_CAPTURE
                        : 0));
        }
    }

    // =========================================
    // BISHOPS
    // =========================================

    U64 bishops = board.pieces[bishop];

    while (bishops)
    {
        int from = pop_lsb(bishops);

        U64 moves =
            get_bishop_attacks(from, st.occ) & ~st.usOcc & st.checkMask;

        if (st.pinned & (1ULL << from))
            moves &= st.pinRay[from];

        while (moves)
        {
            int to = pop_lsb(moves);

            int cap =
                board.piece_at[to];

            list.add(
                encode_move(
                    from,
                    to,
                    bishop,
                    cap,
                    cap != EMPTY
                        ? MF_CAPTURE
                        : 0));
        }
    }

    // =========================================
    // ROOKS
    // =========================================

    U64 rooks = board.pieces[rook];

    while (rooks)
    {
        int from = pop_lsb(rooks);

        U64 moves =
            get_rook_attacks(from, st.occ) & ~st.usOcc & st.checkMask;

        if (st.pinned & (1ULL << from))
            moves &= st.pinRay[from];

        while (moves)
        {
            int to = pop_lsb(moves);

            int cap =
                board.piece_at[to];

            list.add(
                encode_move(
                    from,
                    to,
                    rook,
                    cap,
                    cap != EMPTY
                        ? MF_CAPTURE
                        : 0));
        }
    }

    // =========================================
    // QUEENS
    // =========================================

    U64 queens = board.pieces[queen];

    while (queens)
    {
        int from = pop_lsb(queens);

        U64 moves =
            (get_bishop_attacks(from, st.occ) |
             get_rook_attacks(from, st.occ)) &
            ~st.usOcc & st.checkMask;

        if (st.pinned & (1ULL << from))
            moves &= st.pinRay[from];

        while (moves)
        {
            int to = pop_lsb(moves);

            int cap =
                board.piece_at[to];

            list.add(
                encode_move(
                    from,
                    to,
                    queen,
                    cap,
                    cap != EMPTY
                        ? MF_CAPTURE
                        : 0));
        }
    }
}

// generates only the legal captures in the current position
void generate_captures(const Board &board, MoveList &list)
{
    list.clear();

    GenState st = compute_state(board);

    // =========================================
    // KING CAPTURES
    // =========================================

    {
        int from = st.kingSq;

        U64 moves =
            kingAttacks[from] &
            st.themOcc &
            ~st.danger;

        while (moves)
        {
            int to = pop_lsb(moves);

            list.add(
                encode_move(
                    from,
                    to,
                    st.us == WHITE ? W_K : B_K,
                    board.piece_at[to],
                    MF_CAPTURE));
        }
    }

    // double check => only king moves
    if (bit_count(st.checkers) >= 2)
        return;

    const int pawn =
        st.us == WHITE ? W_P : B_P;

    const int knight =
        st.us == WHITE ? W_N : B_N;

    const int bishop =
        st.us == WHITE ? W_B : B_B;

    const int rook =
        st.us == WHITE ? W_R : B_R;

    const int queen =
        st.us == WHITE ? W_Q : B_Q;

    // =========================================
    // PAWNS
    // =========================================

    U64 pawns = board.pieces[pawn];

    while (pawns)
    {
        int from = pop_lsb(pawns);

        int promoRank =
            st.us == WHITE ? 7 : 0;

        U64 caps =
            pawnAttacks[st.us][from] &
            st.themOcc &
            st.checkMask;

        if (st.pinned & (1ULL << from))
            caps &= st.pinRay[from];

        while (caps)
        {
            int to = pop_lsb(caps);

            int cap = board.piece_at[to];

            if ((to / 8) == promoRank)
            {
                int promos[4] =
                {
                    queen,
                    rook,
                    bishop,
                    knight
                };

                for (int p = 0; p < 4; ++p)
                {
                    list.add(
                        encode_move(
                            from,
                            to,
                            promos[p],
                            cap,
                            MF_PROMO | MF_CAPTURE));
                }
            }
            else
            {
                list.add(
                    encode_move(
                        from,
                        to,
                        pawn,
                        cap,
                        MF_CAPTURE));
            }
        }

        // en passant
        if (board.ep != -1)
        {
            U64 epTargets =
                pawnAttacks[st.us][from] &
                (1ULL << board.ep);

            if (epTargets)
            {
                int epTo = board.ep;

                if (ep_legal(board, st, from, epTo))
                {
                    int capPawn =
                        st.us == WHITE ? B_P : W_P;

                    list.add(
                        encode_move(
                            from,
                            epTo,
                            pawn,
                            capPawn,
                            MF_EP | MF_CAPTURE));
                }
            }
        }
    }

    // =========================================
    // KNIGHTS
    // =========================================

    U64 knights = board.pieces[knight];

    while (knights)
    {
        int from = pop_lsb(knights);

        if (st.pinned & (1ULL << from))
            continue;

        U64 moves =
            knightAttacks[from] &
            st.themOcc &
            st.checkMask;

        while (moves)
        {
            int to = pop_lsb(moves);

            list.add(
                encode_move(
                    from,
                    to,
                    knight,
                    board.piece_at[to],
                    MF_CAPTURE));
        }
    }

    // =========================================
    // BISHOPS
    // =========================================

    U64 bishops = board.pieces[bishop];

    while (bishops)
    {
        int from = pop_lsb(bishops);

        U64 moves =
            get_bishop_attacks(from, st.occ) &
            st.themOcc &
            st.checkMask;

        if (st.pinned & (1ULL << from))
            moves &= st.pinRay[from];

        while (moves)
        {
            int to = pop_lsb(moves);

            list.add(
                encode_move(
                    from,
                    to,
                    bishop,
                    board.piece_at[to],
                    MF_CAPTURE));
        }
    }

    // =========================================
    // ROOKS
    // =========================================

    U64 rooks = board.pieces[rook];

    while (rooks)
    {
        int from = pop_lsb(rooks);

        U64 moves =
            get_rook_attacks(from, st.occ) &
            st.themOcc &
            st.checkMask;

        if (st.pinned & (1ULL << from))
            moves &= st.pinRay[from];

        while (moves)
        {
            int to = pop_lsb(moves);

            list.add(
                encode_move(
                    from,
                    to,
                    rook,
                    board.piece_at[to],
                    MF_CAPTURE));
        }
    }

    // =========================================
    // QUEENS
    // =========================================

    U64 queens = board.pieces[queen];

    while (queens)
    {
        int from = pop_lsb(queens);

        U64 moves =
            (get_bishop_attacks(from, st.occ) |
             get_rook_attacks(from, st.occ)) &
            st.themOcc &
            st.checkMask;

        if (st.pinned & (1ULL << from))
            moves &= st.pinRay[from];

        while (moves)
        {
            int to = pop_lsb(moves);

            list.add(
                encode_move(
                    from,
                    to,
                    queen,
                    board.piece_at[to],
                    MF_CAPTURE));
        }
    }
}
