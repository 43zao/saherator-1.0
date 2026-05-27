#pragma once
#include "board.h"

// Evaluates the given board position.
// Contains constants for piece and square values.

// IMPLEMENTS:
// PeSTO inspired piece values and square tables
// Mid / endgame tapered eval
// King safety evaluation
// Doubled / isolated pawn penalties
// Passed pawn bonuses
// Bishop pair bonus
// Rooks on open files bonus

namespace Eval {
    constexpr int MATE_SCORE = 30000;
    constexpr int INF = 32000;

    //-----------------------------------------
    // PeSTO VALUES
    //-----------------------------------------

    static constexpr int mgValue[6] = {
        82, 337, 365, 477, 1025, 0};

    static constexpr int egValue[6] = {
        94, 281, 297, 512, 936, 0};

    extern int mgTable[12][64];
    extern int egTable[12][64];

    void init_eval();
    int evaluate(const Board& board);
}
