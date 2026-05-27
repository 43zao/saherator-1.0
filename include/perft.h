#pragma once
#include "board.h"
#include "movegen.h"
#include "attacks.h"
#include <chrono>
#include <iostream>

// Standard perft tests for move generation correctness

struct PerftStats
{
    U64 nodes = 0;
    U64 captures = 0;
    U64 ep = 0;
    U64 castles = 0;
    U64 promotions = 0;
    U64 checks = 0;

    void add(const PerftStats &other)
    {
        nodes += other.nodes;
        captures += other.captures;
        ep += other.ep;
        castles += other.castles;
        promotions += other.promotions;
        checks += other.checks;
    }
};

void perft_recursive(Board &board, int depth, PerftStats &stats);

// prints stats per root node
void perft_divide(Board &board, int depth);

// prints only the node count
// used for perft_diff.py testing against stockfish perft
void perft_divide_plain(Board &board, int depth);
