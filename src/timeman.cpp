#include "timeman.h"
#include <algorithm>

namespace TimeMan
{
    int allocate_time(
        const Board &board,
        const TimeControl &tc)
    {
        //----------------------------------------
        // TC not initialized / invalid -> infinite search
        //----------------------------------------
        if (tc.movetime <= 0 &&
            tc.wtime <= 0 &&
            tc.btime <= 0)
        {
            return -1;
        }

        //----------------------------------------
        // Explicit movetime
        //----------------------------------------
        if (tc.movetime > 0)
        {
            return tc.movetime;
        }

        int remaining =
            (board.stm == WHITE)
                ? tc.wtime
                : tc.btime;

        int increment =
            (board.stm == WHITE)
                ? tc.winc
                : tc.binc;

        if (remaining <= 0)
        {
            return MIN_THINK_TIME;
        }

        //----------------------------------------
        // Phase selection
        //----------------------------------------
        double divisor;

        if (board.fullmove < 15)
        {
            divisor = OPENING_DIVISOR;
        }
        else if (board.fullmove < 40)
        {
            divisor = MIDDLEGAME_DIVISOR;
        }
        else
        {
            divisor = ENDGAME_DIVISOR;
        }

        //----------------------------------------
        // Base allocation
        //----------------------------------------
        int thinkTime =
            static_cast<int>(remaining / divisor);

        //----------------------------------------
        // Increment bonus
        //----------------------------------------
        thinkTime +=
            static_cast<int>(increment * INCREMENT_FACTOR);

        //----------------------------------------
        // Reserve protection
        //----------------------------------------
        int reserve;

        if (remaining > MEDIUM_RESERVE_THRESHOLD)
        {
            reserve = LARGE_TIME_RESERVE;
        }
        else if (remaining > LOW_RESERVE_THRESHOLD)
        {
            reserve = MEDIUM_TIME_RESERVE;
        }
        else
        {
            reserve = LOW_TIME_RESERVE;
        }

        int maxSpend = remaining - reserve;

        if (maxSpend < MIN_THINK_TIME)
        {
            maxSpend = MIN_THINK_TIME;
        }

        thinkTime = std::min(thinkTime, maxSpend);

        //----------------------------------------
        // Final floor
        //----------------------------------------
        thinkTime = std::max(thinkTime, MIN_THINK_TIME);

        return thinkTime;
    }
}
