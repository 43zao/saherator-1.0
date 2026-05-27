#include <iostream>
#include <string>

#include "utils.h"
#include "board.h"
#include "movegen.h"
#include "attacks.h"
#include "uci.h"
#include "search.h"
#include "eval.h"
#include "timeman.h"
#include "perft.h"
#include "tests.h"

// Main only runs the UCI loop.
// All logic is handled using the UCI protocol.

// All structures must be initialized for the engine to work properly.
// UCI currently takes care of this, but it should still be noted.

// Current list of init functions to run:

// init_movegen();
// init_between_tables();
// Eval::init_eval();


int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[])
{

    /*  PERFT TESTS

    // run_movegen_init_tests();
    // run_movegen_diagnostics();
    // test_random_bishops();
    // test_attacked_sq();

    init_movegen();
    init_between_tables();
    Board b;

    // ----------------------------
    // CLI mode:
    // ./Saherator "<fen>" depth
    // ----------------------------
    if (argc > 1) {
        std::cout << "TEST BLOCK ENTERED" << std::endl;
        std::string fen = argv[1];
        int depth = std::stoi(argv[2]);

        if (!b.init_from_fen(fen.c_str())) {
            std::cerr << "Invalid FEN\n";
            return 1;
        }

        perft_divide_plain(b, depth);
        return 0;
    }

    b.init_from_fen(STARTFEN);
    perft_divide(b, 6);

    */

    
    // Universal Chess Interface loop

    UCI::loop();

    return 0;
}
