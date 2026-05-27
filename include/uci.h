#pragma once
#include <string>
#include "board.h"

// Implements the Universal Chess Interface protocol.
// The loop handles all internal engine logic, integrating it with UCI commands.
// Allows the engine to connect to GUIs or CLIs.
// Multithreaded and thread-safe.

// CURRENTLY SUPPORTED COMMANDS:
// uci
// isready
// ucinewgame
// position (startpos / fen ...) (moves ...)
// go (depth / movetime / time control / infinite / ponder)
// ponderhit
// stop
// quit
// d    --> debug command, prints the board

namespace UCI
{
    // main UCI loop, waits for commands
    void loop();

    // helpers to cross convert UCI moves with internal engine move representation

    std::string move_to_string(U32 move);
    U32 parse_move(Board &board, const std::string &str);
}
