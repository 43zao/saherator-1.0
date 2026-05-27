#include "board.h"
#include <iostream>
#include <cstring>
#include <sstream>

// zobrist keys
U64 zobPiece[12][64];
U64 zobEp[64];
U64 zobCastling[16];
U64 zobSide;

inline static void init_zobrist()
{
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;

    for (int p = 0; p < 12; ++p)
    {
        for (int sq = 0; sq < 64; ++sq)
            zobPiece[p][sq] = rand64();
    }

    for (int sq = 0; sq < 64; ++sq)
        zobEp[sq] = rand64();

    for (int i = 0; i < 16; ++i)
        zobCastling[i] = rand64();

    zobSide = rand64();
}

inline static U64 compute_zobrist(const Board &board)
{
    U64 hash = 0ULL;

    // Pieces on squares
    for (int p = 0; p < 12; ++p)
    {
        U64 bb = board.pieces[p];
        while (bb)
        {
            int sq = pop_lsb(bb);
            hash ^= zobPiece[p][sq];
        }
    }

    // Castling rights
    hash ^= zobCastling[board.castling];

    // En-passant square
    if (board.ep >= 0 && board.ep < 64)
    {
        hash ^= zobEp[board.ep];
    }

    // Side to move
    if (board.stm == BLACK)
    {
        hash ^= zobSide;
    }

    return hash;
}

// helpers
static inline int file_char_to_idx(char f)
{
    if (f < 'a' || f > 'h')
        return -1;
    return f - 'a';
}
static inline int rank_char_to_idx(char r)
{
    if (r < '1' || r > '8')
        return -1;
    return r - '1';
}
static inline int sq_from_coords(char filec, char rankc)
{
    int f = file_char_to_idx(filec);
    int r = rank_char_to_idx(rankc);
    if (f < 0 || r < 0)
        return -1;
    return r * 8 + f;
}

// map piece char to Piece enum
static inline int piece_from_char(char c)
{
    switch (c)
    {
    case 'P':
        return W_P;
    case 'N':
        return W_N;
    case 'B':
        return W_B;
    case 'R':
        return W_R;
    case 'Q':
        return W_Q;
    case 'K':
        return W_K;
    case 'p':
        return B_P;
    case 'n':
        return B_N;
    case 'b':
        return B_B;
    case 'r':
        return B_R;
    case 'q':
        return B_Q;
    case 'k':
        return B_K;
    default:
        return -1;
    }
}

// board methods

Board::Board()
{
    reset();
}

void Board::reset()
{
    std::memset(pieces, 0, sizeof(pieces));
    std::memset(occ, 0, sizeof(occ));
    std::fill(std::begin(piece_at), std::end(piece_at), EMPTY);

    zobrist = 0ULL;
    fullmove = 1;
    stm = WHITE;
    castling = 0;
    ep = -1;
    halfmove = 0;
    ply = 0;
}

void Board::update_occupancies()
{
    occ[WHITE] = pieces[W_P] | pieces[W_N] | pieces[W_B] |
                 pieces[W_R] | pieces[W_Q] | pieces[W_K];

    occ[BLACK] = pieces[B_P] | pieces[B_N] | pieces[B_B] |
                 pieces[B_R] | pieces[B_Q] | pieces[B_K];

    occ[BOTH] = occ[WHITE] | occ[BLACK];
}

bool Board::init_from_fen(const char *fen_cstr)
{
    if (!fen_cstr) return false;

    init_zobrist();

    reset();
    std::string fen(fen_cstr);
    std::istringstream ss(fen);
    std::string fields[6];

    // read up to 6 fields; missing ones stay empty
    for (int i = 0; i < 6 && ss; ++i)
    {
        ss >> fields[i];
    }

    const std::string &piece_placement = fields[0];
    const std::string &stm_field = fields[1];
    const std::string &cast_field = fields[2];
    const std::string &ep_field = fields[3];
    const std::string &half_field = fields[4];
    const std::string &full_field = fields[5];

    // piece placement
    int rank = 7;
    int file = 0;
    for (char c : piece_placement)
    {
        if (c == '/')
        {
            rank--;
            file = 0;
            continue;
        }
        if (isdigit((unsigned char)c))
        {
            file += c - '0';
            continue;
        }
        if (file > 7 || rank < 0)
            return false;

        int sq = rank * 8 + file;
        int p = piece_from_char(c);
        if (p < 0)
            return false;

        set_bit(pieces[p], sq);
        piece_at[sq] = (U8)p;
        file++;
    }

    // side to move
    if (stm_field == "w")
        stm = WHITE;
    else if (stm_field == "b")
        stm = BLACK;
    else
        return false;

    // castling rights
    castling = 0;
    if (cast_field != "-")
    {
        for (char c : cast_field)
        {
            switch (c)
            {
            case 'K':
                castling |= CASTLE_WK;
                break;
            case 'Q':
                castling |= CASTLE_WQ;
                break;
            case 'k':
                castling |= CASTLE_BK;
                break;
            case 'q':
                castling |= CASTLE_BQ;
                break;
            default:
                return false;
            }
        }
    }

    // en-passant
    if (ep_field == "-" || ep_field.empty())
        ep = -1;
    else
    {
        if (ep_field.size() != 2)
            return false;
        int f = file_char_to_idx(ep_field[0]);
        int r = rank_char_to_idx(ep_field[1]);
        if (f < 0 || r < 0)
            return false;
        ep = (S8)(r * 8 + f);
    }

    // halfmove
    if (half_field.empty())
        halfmove = 0;
    else
    {
        char *endptr = nullptr;
        long hm = std::strtol(half_field.c_str(), &endptr, 10);
        if (endptr == half_field.c_str() || hm < 0 || hm > 255)
            return false;
        halfmove = (U8)hm;
    }

    // fullmove
    if (full_field.empty())
        fullmove = 1;
    else
    {
        char *endptr = nullptr;
        long fm = std::strtol(full_field.c_str(), &endptr, 10);
        if (endptr == full_field.c_str() || fm < 1 || fm > 65535)
            return false;
        fullmove = (U16)fm;
    }

    zobrist = compute_zobrist(*this);
    update_occupancies();
    return true;
}

std::string Board::to_fen() const
{
    std::string out;

    // piece placement
    for (int rank = 7; rank >= 0; --rank)
    {
        int empties = 0;
        for (int file = 0; file < 8; ++file)
        {
            int sq = rank * 8 + file;
            U8 p = piece_at[sq];
            if (p == EMPTY)
            {
                ++empties;
            }
            else
            {
                if (empties)
                {
                    out += char('0' + empties);
                    empties = 0;
                }
                static const char pieceChars[12] = {
                    'P', 'N', 'B', 'R', 'Q', 'K',
                    'p', 'n', 'b', 'r', 'q', 'k'};
                out.push_back(pieceChars[p]);
            }
        }
        if (empties)
            out += char('0' + empties);
        if (rank)
            out.push_back('/');
    }

    // side to move
    out += ' ';
    out += (stm == WHITE) ? 'w' : 'b';

    // castling
    out += ' ';
    bool any = false;
    if (castling & CASTLE_WK)
    {
        out.push_back('K');
        any = true;
    }
    if (castling & CASTLE_WQ)
    {
        out.push_back('Q');
        any = true;
    }
    if (castling & CASTLE_BK)
    {
        out.push_back('k');
        any = true;
    }
    if (castling & CASTLE_BQ)
    {
        out.push_back('q');
        any = true;
    }
    if (!any)
        out.push_back('-');

    // en-passant
    out += ' ';
    if (ep == -1)
        out += '-';
    else
    {
        int f = ep % 8;
        int r = ep / 8;
        out.push_back(char('a' + f));
        out.push_back(char('1' + r));
    }

    // halfmove & fullmove
    out += ' ';
    out += std::to_string((int)halfmove);
    out += ' ';
    out += std::to_string((int)fullmove);

    return out;
}

void Board::print_board() const
{
    for (int r = 7; r >= 0; --r)
    {
        std::cout << r + 1 << " ";
        for (int f = 0; f < 8; ++f)
        {
            int sq = r * 8 + f;
            U8 p = piece_at[sq];
            char c = '.';
            if (p != EMPTY)
            {
                static const char *pieceChars = "PNBRQKpnbrqk";
                c = pieceChars[p];
            }
            std::cout << c << " ";
        }
        std::cout << "\n";
    }
    std::cout << "  a b c d e f g h\n";
}

bool Board::make_move(U32 mv)
{
    int from = move_from(mv);
    int to = move_to(mv);
    int piece = move_piece(mv);
    int captured = move_captured(mv);
    U32 flags = move_flags(mv);

    Color us = stm;

    // save undo
    Undo u{};
    u.move = mv;
    u.moved_piece = (flags & MF_PROMO) ? (us == WHITE ? W_P : B_P) : piece;
    u.captured = captured;
    u.castling = castling;
    u.ep = ep;
    u.halfmove = halfmove;
    u.zobrist = zobrist;
    undo_stack[ply++] = u;

    // halfmove clock
    if (piece == W_P || piece == B_P || captured != EMPTY)
        halfmove = 0;
    else
        halfmove++;

    // fullmove increment
    if (us == BLACK)
        fullmove++;

    // remove captured piece
    if (captured != EMPTY && to >= 0 && to < 64)
    {
        clear_bit(pieces[captured], to);
        piece_at[to] = EMPTY;
        zobrist ^= zobPiece[captured][to];
    }

    // move piece
    if (flags & MF_PROMO)
    {
        int pawn = (us == WHITE ? W_P : B_P);
        int promo_piece = move_piece(mv);

        // remove pawn from source square
        clear_bit(pieces[pawn], from);
        piece_at[from] = EMPTY;
        zobrist ^= zobPiece[pawn][from];

        // place promoted piece on target square
        set_bit(pieces[promo_piece], to);
        piece_at[to] = promo_piece;
        zobrist ^= zobPiece[promo_piece][to];
    }
    else
    {
        // normal move
        clear_bit(pieces[piece], from);
        set_bit(pieces[piece], to);

        piece_at[from] = EMPTY;
        piece_at[to] = piece;

        zobrist ^= zobPiece[piece][from];
        zobrist ^= zobPiece[piece][to];
    }

    // special moves
    // en passant
    if (flags & MF_EP)
    {
        int ep_sq = (us == WHITE) ? to - 8 : to + 8;
        if (ep_sq >= 0 && ep_sq < 64)
        {
            int ep_captured = (us == WHITE) ? B_P : W_P;
            clear_bit(pieces[ep_captured], ep_sq);
            piece_at[ep_sq] = EMPTY;
            zobrist ^= zobPiece[ep_captured][ep_sq];
        }
    }

    // castling
    if (flags & MF_CASTLE)
    {
        if (to == 6)
        {   // white kingside
            clear_bit(pieces[W_R], 7);
            set_bit(pieces[W_R], 5);
            piece_at[7] = EMPTY;
            piece_at[5] = W_R;
            zobrist ^= zobPiece[W_R][7] ^ zobPiece[W_R][5];
        }
        else if (to == 2)
        {   // white queenside
            clear_bit(pieces[W_R], 0);
            set_bit(pieces[W_R], 3);
            piece_at[0] = EMPTY;
            piece_at[3] = W_R;
            zobrist ^= zobPiece[W_R][0] ^ zobPiece[W_R][3];
        }
        else if (to == 62)
        {   // black kingside
            clear_bit(pieces[B_R], 63);
            set_bit(pieces[B_R], 61);
            piece_at[63] = EMPTY;
            piece_at[61] = B_R;
            zobrist ^= zobPiece[B_R][63] ^ zobPiece[B_R][61];
        }
        else if (to == 58)
        {   // black queenside
            clear_bit(pieces[B_R], 56);
            set_bit(pieces[B_R], 59);
            piece_at[56] = EMPTY;
            piece_at[59] = B_R;
            zobrist ^= zobPiece[B_R][56] ^ zobPiece[B_R][59];
        }
    }

    // update castling
    zobrist ^= zobCastling[castling];
    switch (from)
    {
    case 4:
        castling &= ~(CASTLE_WK | CASTLE_WQ);
        break;
    case 60:
        castling &= ~(CASTLE_BK | CASTLE_BQ);
        break;
    case 0:
        castling &= ~CASTLE_WQ;
        break;
    case 7:
        castling &= ~CASTLE_WK;
        break;
    case 56:
        castling &= ~CASTLE_BQ;
        break;
    case 63:
        castling &= ~CASTLE_BK;
        break;
    }
    switch (to)
    {
    case 0:
        castling &= ~CASTLE_WQ;
        break;
    case 7:
        castling &= ~CASTLE_WK;
        break;
    case 56:
        castling &= ~CASTLE_BQ;
        break;
    case 63:
        castling &= ~CASTLE_BK;
        break;
    }
    zobrist ^= zobCastling[castling];

    // update en-passant
    if (ep >= 0 && ep < 64) zobrist ^= zobEp[ep];
    ep = -1;
    if (flags & MF_PAWN2) ep = (us == WHITE) ? from + 8 : from - 8;
    if (ep >= 0 && ep < 64) zobrist ^= zobEp[ep];

    // change side
    stm = opposite(stm);
    zobrist ^= zobSide;

    update_occupancies();
    return true;
}

void Board::unmake_move()
{
    if (ply == 0) return;

    Undo &u = undo_stack[--ply];
    U32 mv = u.move;

    Color us = stm;     // side that just moved

    // restore meta
    castling = u.castling;
    ep = u.ep;
    halfmove = u.halfmove;
    zobrist = u.zobrist;
    stm = opposite(stm);

    if (us == BLACK) fullmove--;

    int from = move_from(mv);
    int to = move_to(mv);
    int piece = move_piece(mv);
    int captured = move_captured(mv);
    U32 flags = move_flags(mv);

    // undo promotion
    if (flags & MF_PROMO)
    {
        int promo_piece = piece;    // encoded piece is promoted piece
        int pawn = u.moved_piece;   // original moving piece was pawn

        clear_bit(pieces[promo_piece], to);
        piece_at[to] = EMPTY;

        set_bit(pieces[pawn], from);
        piece_at[from] = pawn;
    }
    else
    {
        // normal move back
        clear_bit(pieces[u.moved_piece], to);
        set_bit(pieces[u.moved_piece], from);

        piece_at[from] = u.moved_piece;
        piece_at[to] = EMPTY;
    }

    // restore captured piece
    if (captured != EMPTY)
    {
        int cap_sq = (flags & MF_EP)
                         ? ((us == WHITE) ? to + 8 : to - 8)
                         : to;

        set_bit(pieces[captured], cap_sq);
        piece_at[cap_sq] = captured;
    }

    // undo rook movement in castling
    if (flags & MF_CASTLE)
    {
        switch (to)
        {
        case 6:
            clear_bit(pieces[W_R], 5);
            set_bit(pieces[W_R], 7);
            piece_at[5] = EMPTY;
            piece_at[7] = W_R;
            break;

        case 2:
            clear_bit(pieces[W_R], 3);
            set_bit(pieces[W_R], 0);
            piece_at[3] = EMPTY;
            piece_at[0] = W_R;
            break;

        case 62:
            clear_bit(pieces[B_R], 61);
            set_bit(pieces[B_R], 63);
            piece_at[61] = EMPTY;
            piece_at[63] = B_R;
            break;

        case 58:
            clear_bit(pieces[B_R], 59);
            set_bit(pieces[B_R], 56);
            piece_at[59] = EMPTY;
            piece_at[56] = B_R;
            break;
        }
    }

    update_occupancies();
}

bool Board::make_null_move()
{
    Undo &u = undo_stack[ply++];

    u.move = 0;
    u.moved_piece = EMPTY;
    u.captured = EMPTY;
    u.castling = castling;
    u.ep = ep;
    u.halfmove = halfmove;
    u.zobrist = zobrist;

    // remove EP hash
    if (ep >= 0 && ep < 64) zobrist ^= zobEp[ep];

    ep = -1;

    // flip side
    stm = opposite(stm);
    zobrist ^= zobSide;

    halfmove++;

    if (stm == WHITE) fullmove++;

    return true;
}

void Board::unmake_null_move()
{
    Undo &u = undo_stack[--ply];

    zobrist = u.zobrist;
    castling = u.castling;
    ep = u.ep;
    halfmove = u.halfmove;

    stm = opposite(stm);

    if (stm == BLACK) fullmove--;
}
