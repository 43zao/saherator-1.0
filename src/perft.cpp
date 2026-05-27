#include "perft.h"

// helper: square index (0..63) -> "a1".."h8"
inline static std::string square_to_str(int sq)
{
    int f = sq % 8;
    int r = sq / 8;
    return std::string(1, 'a' + f) + std::string(1, '1' + r);
}

inline static std::string move_to_string(U32 m)
{
    int from = move_from(m);
    int to = move_to(m);
    int piece = move_piece(m);
    int flags = move_flags(m);

    std::string s = square_to_str(from) + square_to_str(to);

    // promotion
    if (flags & MF_PROMO)
    {
        static const char promo_chars[12] = {
            'P', 'N', 'B', 'R', 'Q', 'K', 'p', 'n', 'b', 'r', 'q', 'k'};
        s += char(tolower(promo_chars[piece]));     // always lowercase for promoted piece
    }

    return s;
}

// recursive perft
void perft_recursive(Board &board, int depth, PerftStats &stats)
{
    if (depth == 0)
    {
        stats.nodes++;
        return;
    }

    MoveList moves;
    generate_legal_fast(board, moves);

    for (int i = 0; i < moves.count; i++)
    {
        Move mv = moves.moves[i];
        // classify move
        int flags = move_flags(mv);
        if (flags & MF_CAPTURE)
            stats.captures++;
        if (flags & MF_EP)
            stats.ep++;
        if (flags & MF_CASTLE)
            stats.castles++;
        if (flags & MF_PROMO)
            stats.promotions++;

        board.make_move(mv);

        Color movedSide = opposite(board.stm);
        int oppKing = find_king_sq(board, board.stm);
        if (square_attacked(board, oppKing, movedSide))
            stats.checks++;

        perft_recursive(board, depth - 1, stats);
        board.unmake_move();
    }
}

// divide with stats and timing
void perft_divide(Board &board, int depth)
{
    if (depth < 1) return;

    MoveList moves;
    generate_legal_fast(board, moves);

    U64 totalNodes = 0;
    PerftStats totalStats;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < moves.count; i++)
    {
        Move mv = moves.moves[i];
        board.make_move(mv);

        PerftStats stats;
        perft_recursive(board, depth - 1, stats);

        board.unmake_move();

        totalNodes += stats.nodes;
        totalStats.add(stats);

        std::cout << move_to_string(mv) << ": " << stats.nodes
                  << "  (Captures: " << stats.captures
                  << ", E.p.: " << stats.ep
                  << ", Castles: " << stats.castles
                  << ", Promotions: " << stats.promotions
                  << ", Checks: " << stats.checks << ")\n";
    }

    auto end = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();

    std::cout << "\nTotal nodes: " << totalNodes << "\n";
    std::cout << "Captures: " << totalStats.captures
              << ", E.p.: " << totalStats.ep
              << ", Castles: " << totalStats.castles
              << ", Promotions: " << totalStats.promotions
              << ", Checks: " << totalStats.checks << "\n";

    std::cout << "Time: " << elapsed << "s, Nodes/sec: "
              << (double)totalNodes / elapsed << "\n";
}

void perft_divide_plain(Board &board, int depth)
{
    if (depth < 1) return;

    MoveList moves;
    generate_legal_fast(board, moves);

    U64 totalNodes = 0;

    for (int i = 0; i < moves.count; i++)
    {
        Move mv = moves.moves[i];
        board.make_move(mv);

        PerftStats stats;
        perft_recursive(board, depth - 1, stats);

        board.unmake_move();

        totalNodes += stats.nodes;

        std::cout << move_to_string(mv)
                  << ": "
                  << stats.nodes
                  << "\n";
    }

    std::cout << "\nNodes searched: "
              << totalNodes
              << "\n";
}
