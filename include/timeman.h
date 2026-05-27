#pragma once
#include "board.h"

// Manages time usage for the engine depending on time control / UCI command.
// Defines constants which control where most of the time is spent.

namespace TimeMan
{
    struct TimeControl
    {
        int wtime = -1;
        int btime = -1;

        int winc = 0;
        int binc = 0;

        int movetime = -1;
    };

    // opening: no book so solid time
    constexpr double OPENING_DIVISOR = 30.0;

    // middlegame: spend most time
    constexpr double MIDDLEGAME_DIVISOR = 25.0;

    // endgame: presummably less pieces so needs less time
    constexpr double ENDGAME_DIVISOR = 35.0;

    // how much of increment to use
    constexpr double INCREMENT_FACTOR = 0.75;

    // emergency reserves - activate at threshold

    constexpr int MEDIUM_RESERVE_THRESHOLD = 60000;     // 60 sec
    constexpr int LOW_RESERVE_THRESHOLD = 10000;        // 10 sec

    constexpr int LARGE_TIME_RESERVE = 10000;   // 10 sec
    constexpr int MEDIUM_TIME_RESERVE = 3000;   // 3 sec
    constexpr int LOW_TIME_RESERVE = 1000;      // 1 secs

    // never think less than this
    constexpr int MIN_THINK_TIME = 50;

    int allocate_time(const Board &board, const TimeControl &tc);
}