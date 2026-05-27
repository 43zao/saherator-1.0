#pragma once

#include "board.h"
#include "movegen.h"
#include "attacks.h"
#include "eval.h"
#include "timeman.h"
#include <array>

// Searches through all possible moves in a given position to find the best one.
// Returns results for a set time / depth.

// IMPLEMENTS:
// Alpha-beta pruning negamax search
// Move ordering
// Transposition tables
// Quiescence search with delta pruning
// Principal variation search
// Late move reductions
// Check extensions
// Null move pruning
// Killer moves
// History heuristics
// Aspiration windows
// Iterative deepening
// Time management
// Pondering
// 50 move draw / stalemate / checkmate detection

namespace Search
{
    // Transposition table size
    constexpr int TT_SIZE = 1 << 25;    // 2^25 = 33,554,432 entries = 512MB

    enum TTFlag : U8
    {
        TT_EXACT,
        TT_ALPHA,
        TT_BETA
    };

    struct TTEntry
    {
        U64 key = 0;            // Zobrist key of the position
        Move bestMove = 0;
        S16 score = 0;          // Eval score
        U8 flag = TT_EXACT;
        S8 depth = -1;
    };

    struct SearchResult
    {
        Move bestMove;
        Move ponderMove;        // Proposed best response for opponent
        int score;
        int depth;
    };

    // quiescence search (used at depth = 0)
    int quiescence(
        Board &board,
        int alpha,
        int beta);

    // negamax algorithm for looking through the move tree
    int negamax(
        Board &board,
        int depth,
        int alpha,
        int beta);

    // returns the best engine move in the position
    SearchResult find_best_move(
        Board &board,
        int maxDepth = 64);

    // how long in milliseconds until search is stopped
    void set_search_time(int ms);

    // lets the engine search during opponent's move
    void set_pondering(bool enabled);

    // returns pondering state
    bool is_pondering();

    // sets the time control to be used for determining search time
    void set_time_control(const TimeMan::TimeControl &tc);

    // returns set time control
    const TimeMan::TimeControl &get_time_control();
    
    // used when ponder move hits to seamlessly continue the search
    void recompute_time(const Board &board);

    // immediately stops searching
    void stop();
}
