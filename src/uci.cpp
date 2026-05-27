#include "uci.h"
#include "movegen.h"
#include "search.h"
#include "timeman.h"
#include "attacks.h"
#include "eval.h"

#include <sstream>
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>

namespace UCI
{
    constexpr int MAX_DEPTH = 64;

    static std::thread searchThread;
    static std::mutex coutMutex;

    // UCI output (thread-safe)
    static void uci_print(const std::string &s)
    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << s << std::endl;
    }

    static char file_char(int sq)
    {
        return 'a' + (sq % 8);
    }

    static char rank_char(int sq)
    {
        return '1' + (sq / 8);
    }

    std::string move_to_string(U32 move)
    {
        if (!move) return "0000";

        int from = move_from(move);
        int to = move_to(move);

        std::string s;
        s += file_char(from);
        s += rank_char(from);
        s += file_char(to);
        s += rank_char(to);

        if (move_flags(move) & MF_PROMO)
        {
            int piece = move_piece(move);

            switch (piece)
            {
            case W_Q:
            case B_Q:
                s += 'q';
                break;

            case W_R:
            case B_R:
                s += 'r';
                break;

            case W_B:
            case B_B:
                s += 'b';
                break;

            case W_N:
            case B_N:
                s += 'n';
                break;
            }
        }

        return s;
    }

    U32 parse_move(Board &board, const std::string &str)
    {
        // uses movegen trick to parse
        MoveList list;
        generate_legal_fast(board, list);

        for (int i = 0; i < list.count; i++)
        {
            U32 move = list.moves[i];

            if (move_to_string(move) == str)
                return move;
        }

        return 0;
    }

    // meant to be called in a separate thread
    // searches for the best move
    static void search_worker(Board board, int depth)
    {
        Search::SearchResult result =
            Search::find_best_move(board, depth);

        std::string output;

        if (result.bestMove)
        {
            output = "bestmove " + move_to_string(result.bestMove);

            if (result.ponderMove)
            {
                output += " ponder " + move_to_string(result.ponderMove);
            }
        }
        else
        {
            output = "bestmove 0000";
        }

        uci_print(output);
    }

    // stops search safely
    static void stop_search_thread()
    {
        Search::stop();

        if (searchThread.joinable())
            searchThread.join();
    }

    //--------------------------------------------------
    // POSITION
    //--------------------------------------------------

    static void handle_position(Board &board, std::istringstream &iss)
    {
        std::string token;
        iss >> token;

        if (token == "startpos")
        {
            board.init_from_fen(STARTFEN);
        }
        else if (token == "fen")
        {
            std::string fen;
            std::string part;

            for (int i = 0; i < 6 && iss >> part; i++)
            {
                if (part == "moves") break;

                if (!fen.empty()) fen += " ";

                fen += part;
            }

            board.init_from_fen(fen.c_str());
        }

        while (iss >> token)
        {
            if (token == "moves") continue;

            U32 move = parse_move(board, token);

            if (move)
                board.make_move(move);
            else
                std::cerr << "Illegal move: " << token << "\n";
        }
    }

    //--------------------------------------------------
    // GO
    //--------------------------------------------------

    static void handle_go(Board &board, std::istringstream &iss)
    {
        TimeMan::TimeControl tc;

        int depth = MAX_DEPTH;
        bool ponder = false;

        std::string token;

        while (iss >> token)
        {
            if (token == "depth")
            {
                iss >> depth;
            }
            else if (token == "movetime")
            {
                iss >> tc.movetime;
            }
            else if (token == "wtime")
            {
                iss >> tc.wtime;
            }
            else if (token == "btime")
            {
                iss >> tc.btime;
            }
            else if (token == "winc")
            {
                iss >> tc.winc;
            }
            else if (token == "binc")
            {
                iss >> tc.binc;
            }
            else if (token == "infinite")
            {
                tc.movetime = -1;
            }
            else if (token == "ponder")
            {
                ponder = true;
            }
        }

        // stop old search cleanly
        stop_search_thread();

        // store full time control in search (needed for ponderhit recompute)
        Search::set_time_control(tc);

        // compute initial search time
        Search::set_search_time(TimeMan::allocate_time(board, tc));

        // set pondering state
        Search::set_pondering(ponder);

        // launch search thread
        searchThread = std::thread(search_worker, board, depth);
    }

    //--------------------------------------------------
    // MAIN LOOP
    //--------------------------------------------------

    void loop()
    {
        // IMPORTANT: initializes all data structures
        init_movegen();
        init_between_tables();
        Eval::init_eval();

        Board board;
        board.init_from_fen(STARTFEN);

        std::string line;

        while (std::getline(std::cin, line))
        {
            std::istringstream iss(line);

            std::string command;
            iss >> command;

            //--------------------------------------------------
            // UCI INIT
            //--------------------------------------------------

            if (command == "uci")
            {
                uci_print("id name saherator");
                uci_print("id author 43zao");
                uci_print("option name Ponder type check default true");
                uci_print("uciok");
            }

            //--------------------------------------------------
            // READY
            //--------------------------------------------------

            else if (command == "isready")
            {
                uci_print("readyok");
            }

            //--------------------------------------------------
            // NEW GAME
            //--------------------------------------------------

            else if (command == "ucinewgame")
            {
                stop_search_thread();
                board.init_from_fen(STARTFEN);
            }

            //--------------------------------------------------
            // POSITION
            //--------------------------------------------------

            else if (command == "position")
            {
                handle_position(board, iss);
            }

            //--------------------------------------------------
            // GO
            //--------------------------------------------------

            else if (command == "go")
            {
                handle_go(board, iss);
            }

            //--------------------------------------------------
            // STOP
            //--------------------------------------------------

            else if (command == "stop")
            {
                stop_search_thread();
            }

            //--------------------------------------------------
            // PONDERHIT
            //--------------------------------------------------

            else if (command == "ponderhit")
            {
                Search::set_pondering(false);
                // search thread already running
                // it will automatically switch to timed mode
                Search::recompute_time(board);
            }

            //--------------------------------------------------
            // QUIT
            //--------------------------------------------------

            else if (command == "quit")
            {
                stop_search_thread();
                break;
            }

            //--------------------------------------------------
            // DEBUG
            //--------------------------------------------------

            else if (command == "d")
            {
                board.print_board();
                uci_print("\nFEN: " + board.to_fen());
            }
        }
    }

}
