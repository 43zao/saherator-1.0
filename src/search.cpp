#include "search.h"
#include <chrono>
#include <atomic>
#include <iostream>
#include <cstring>
#include <array>
#include <algorithm>

namespace Search
{
    static U64 nodes = 0;

    static std::chrono::steady_clock::time_point searchStart;
    static std::atomic<int> allocatedTimeMs = 0;
    static std::atomic<bool> stopSearch = false;
    static std::atomic<bool> pondering = false;
    static TimeMan::TimeControl currentTC;

    static std::array<TTEntry, TT_SIZE> transpositionTable;

    static Move killerMoves[Board::MAX_PLY][2];
    static int historyTable[64][64];

    static inline int tt_index(U64 key)
    {
        return key & (TT_SIZE - 1);
    }

    //--------------------------------------------------
    // TIME + MODE CONTROL
    //--------------------------------------------------

    void set_search_time(int ms)
    {
        allocatedTimeMs = ms;
    }

    void set_pondering(bool enabled)
    {
        pondering = enabled;
    }

    bool is_pondering()
    {
        return pondering.load();
    }

    void set_time_control(const TimeMan::TimeControl &tc)
    {
        currentTC = tc;
    }

    const TimeMan::TimeControl &get_time_control()
    {
        return currentTC;
    }

    void stop()
    {
        stopSearch = true;
    }

    void reset_search_clock()
    {
        searchStart = std::chrono::steady_clock::now();
        nodes = 0;
    }

    void recompute_time(const Board &board)
    {
        int thinkTime = TimeMan::allocate_time(board, currentTC);
        set_search_time(thinkTime);
        reset_search_clock();
    }

    //--------------------------------------------------
    // TIME CHECK
    //--------------------------------------------------

    static inline void check_time()
    {
        if (stopSearch) return;

        if (pondering.load()) return;

        if (allocatedTimeMs <= 0) return;

        auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - searchStart)
                .count();

        if (elapsed >= allocatedTimeMs) stopSearch = true;
    }

    //--------------------------------------------------
    // POSITION UTILITIES
    //--------------------------------------------------

    static bool is_50_move_draw(const Board &board)
    {
        return board.halfmove >= 100;
    }

    static bool is_repetition(const Board &board)
    {
        int count = 1;
        int limit = board.ply - board.halfmove;

        for (int i = board.ply - 2; i >= limit; i -= 2)
        {
            if (board.undo_stack[i].zobrist == board.zobrist)
            {
                if (++count >= 3)
                    return true;
            }
        }

        return false;
    }

    static inline bool has_non_pawn_material(const Board& board, Color side)
    {
        if (side == WHITE)
        {
            return
                board.pieces[W_N] |
                board.pieces[W_B] |
                board.pieces[W_R] |
                board.pieces[W_Q];
        }

        return
            board.pieces[B_N] |
            board.pieces[B_B] |
            board.pieces[B_R] |
            board.pieces[B_Q];
    }

    //--------------------------------------------------
    // MOVE ORDERING
    //--------------------------------------------------

    static inline int score_move(
        Move move,
        Move ttMove,
        Move killer1,
        Move killer2)
    {
        if (move == ttMove)
            return 1000000;

        int from = move_from(move);
        int to   = move_to(move);

        int captured = move_captured(move);

        if (captured != EMPTY)
        {
            // MVV-LVA
            return 900000 + captured * 10 - move_piece(move);
        }

        // killer moves

        if (move == killer1)
            return 800000;

        if (move == killer2)
            return 799000;

        // history heuristics
        
        return historyTable[from][to];
    }

    static void order_moves(Board &board, MoveList &moves, Move ttMove)
    {
        Move killer1 = killerMoves[board.ply][0];
        Move killer2 = killerMoves[board.ply][1];

        std::sort(moves.moves,
            moves.moves + moves.count,
            [&](Move a, Move b)
            {
                return score_move(a, ttMove, killer1, killer2) >
                        score_move(b, ttMove, killer1, killer2);
            });
    }

    //--------------------------------------------------
    // QUIESCENCE
    //--------------------------------------------------

    int quiescence(Board &board, int alpha, int beta)
    {
        nodes++;
        if ((nodes & 2047) == 0) check_time();

        if (stopSearch) return 0;

        if (is_repetition(board) || is_50_move_draw(board)) return 0;

        // transposition table probe
        TTEntry &entry = transpositionTable[tt_index(board.zobrist)];

        if (entry.key == board.zobrist && entry.depth >= 0)
        {
            int score = entry.score;

            // mate score normalization
            if (score > Eval::MATE_SCORE - 1000)
                score -= board.ply;

            if (score < -Eval::MATE_SCORE + 1000)
                score += board.ply;

            if (entry.flag == TT_EXACT)
                return score;

            if (entry.flag == TT_ALPHA && score <= alpha)
                return score;

            if (entry.flag == TT_BETA && score >= beta)
                return score;
        }

        int originalAlpha = alpha;

        int standPat = Eval::evaluate(board);

        // delta pruning - if adding a queen is not enough, stop searching
        constexpr int DELTA = Eval::mgValue[W_Q];

        if (standPat < alpha - DELTA) return alpha;

        if (standPat >= beta) return beta;

        if (standPat > alpha) alpha = standPat;

        // captures-only generation
        MoveList moves;
        generate_captures(board, moves);

        Move ttMove = (entry.key == board.zobrist) ? entry.bestMove : 0;

        order_moves(board, moves, ttMove);

        Move bestMove = 0;

        for (int i = 0; i < moves.count; i++)
        {
            Move move = moves.moves[i];

            if (move_captured(move) == EMPTY) continue;

            if (!board.make_move(move)) continue;

            int score = -quiescence(board, -beta, -alpha);

            board.unmake_move();

            if (score > alpha)
            {
                alpha = score;
                bestMove = move;
            }

            if (alpha >= beta) break;
        }

        TTFlag flag = TT_EXACT;

        if (alpha <= originalAlpha) flag = TT_ALPHA;
        else if (alpha >= beta) flag = TT_BETA;

        // transposition table store
        if (0 >= entry.depth || entry.key != board.zobrist)
        {
            entry.key = board.zobrist;
            entry.depth = 0;

            int storeScore = alpha;
            if (storeScore > Eval::MATE_SCORE - 1000)
                storeScore += board.ply;
            if (storeScore < -Eval::MATE_SCORE + 1000)
                storeScore -= board.ply;
            entry.score = storeScore;

            entry.flag = flag;
            entry.bestMove = bestMove;
        }

        return alpha;
    }

    //--------------------------------------------------
    // NEGAMAX
    //--------------------------------------------------

    int negamax(Board &board, int depth, int alpha, int beta)
    {
        nodes++;
        if ((nodes & 2047) == 0) check_time();

        if (stopSearch) return 0;

        if (is_repetition(board) || is_50_move_draw(board)) return 0;

        // transposition table probe
        TTEntry &entry = transpositionTable[tt_index(board.zobrist)];

        if (entry.key == board.zobrist && entry.depth >= depth)
        {
            int score = entry.score;

            // mate score normalization

            if (score > Eval::MATE_SCORE - 1000)
                score -= board.ply;

            if (score < -Eval::MATE_SCORE + 1000)
                score += board.ply;

            if (entry.flag == TT_EXACT)
                return score;

            if (entry.flag == TT_ALPHA && score <= alpha)
                return score;

            if (entry.flag == TT_BETA && score >= beta)
                return score;
        }

        alpha = std::max(alpha, -Eval::MATE_SCORE + board.ply);
        beta  = std::min(beta, Eval::MATE_SCORE - board.ply - 1);

        if (alpha >= beta) return alpha;

        // null move pruning

        bool inCheck = is_in_check(board);
        if (
            depth >= 4 &&
            !inCheck &&
            board.ply > 0 &&
            has_non_pawn_material(board, board.stm)
        )
        {
            int R = 2 + depth / 6;

            board.make_null_move();

            int score = -negamax(
                board,
                depth - 1 - R,
                -beta,
                -beta + 1);

            board.unmake_null_move();

            if (stopSearch) return 0;

            if (score >= beta) return beta;
        }

        // quiescence

        if (depth == 0) return quiescence(board, alpha, beta);

        int originalAlpha = alpha;
        MoveList moves;
        generate_legal_fast(board, moves);

        // checkmate / stalemate detection

        if (moves.count == 0)
        {
            int kingSq = find_king_sq(board, board.stm);

            if (square_attacked(board, kingSq, opposite(board.stm)))
                return -Eval::MATE_SCORE + board.ply;

            return 0;
        }

        Move ttMove = (entry.key == board.zobrist) ? entry.bestMove : 0;

        // move ordering

        order_moves(board, moves, ttMove);

        int bestScore = -Eval::INF;
        Move bestMove = 0;

        for (int i = 0; i < moves.count; i++)
        {
            Move move = moves.moves[i];

            if (!board.make_move(move)) continue;

            bool tactical =
                move_captured(move) != EMPTY ||
                (move_flags(move) & MF_PROMO);
            inCheck = is_in_check(board);

            int extension = 0;
            // check extension
            if (inCheck) extension = 1;

            int reduction = 0;
            // late move reductions
            if (!inCheck &&
                !tactical &&
                depth >= 3 &&
                i >= 4)
            {
                reduction = 1;

                // slightly stronger reductions deeper
                if (depth >= 5 && i >= 8)
                    reduction = 2;
            }

            int score;
            // principal variation search - full search for first move
            if (i == 0)
            {
                score = -negamax(
                    board,
                    depth - 1 + extension,
                    -beta,
                    -alpha);
            }
            else
            {
                // reduced null-window search
                score = -negamax(
                    board,
                    depth - 1 + extension - reduction,
                    -alpha - 1,
                    -alpha);

                // if reduced search improves alpha,
                // re-search at normal depth
                if (score > alpha)
                {
                    score = -negamax(
                        board,
                        depth - 1 + extension,
                        -alpha - 1,
                        -alpha);

                    // if still improving alpha,
                    // do full-window search
                    if (score > alpha && score < beta)
                    {
                        score = -negamax(
                            board,
                            depth - 1 + extension,
                            -beta,
                            -alpha);
                    }
                }
            }
            board.unmake_move();

            if (score > bestScore)
            {
                bestScore = score;
                bestMove = move;
            }

            if (score > alpha) alpha = score;

            // beta cutoff
            if (alpha >= beta)
            {
                if (!tactical)
                // killer move + history heuristic updates
                {
                    if (killerMoves[board.ply][0] != move)
                    {
                        killerMoves[board.ply][1] = killerMoves[board.ply][0];
                        killerMoves[board.ply][0] = move;
                    }

                    int &h = historyTable[move_from(move)][move_to(move)];

                    // maximum score for history table entry
                    static constexpr int MAX_HISTORY = 16384;

                    int bonus = depth * depth;

                    h += bonus - h * std::abs(bonus) / MAX_HISTORY;
                }

                break;
            }
        }

        TTFlag flag = TT_EXACT;

        if (bestScore <= originalAlpha) flag = TT_ALPHA;
        else if (bestScore >= beta) flag = TT_BETA;

        // transposition table store
        if (entry.key != board.zobrist || depth >= entry.depth)
        {
            entry.key = board.zobrist;
            entry.depth = depth;

            int storeScore = bestScore;
            if (storeScore > Eval::MATE_SCORE - 1000)
                storeScore += board.ply;
            if (storeScore < -Eval::MATE_SCORE + 1000)
                storeScore -= board.ply;
            entry.score = storeScore;

            entry.flag = flag;
            entry.bestMove = bestMove;
        }

        return bestScore;
    }

    //--------------------------------------------------
    // ROOT SEARCH
    //--------------------------------------------------

    SearchResult search_root(Board &board, int depth, int alpha, int beta)
    {

        SearchResult result{};
        result.bestMove = 0;
        result.score = -Eval::INF;

        MoveList moves;
        generate_legal_fast(board, moves);

        TTEntry &entry = transpositionTable[tt_index(board.zobrist)];

        Move ttMove = (entry.key == board.zobrist) ? entry.bestMove : 0;

        // move ordering

        order_moves(board, moves, ttMove);

        for (int i = 0; i < moves.count; i++)
        {
            Move move = moves.moves[i];

            if (!board.make_move(move)) continue;

            int score = -negamax(board, depth - 1, -beta, -alpha);

            board.unmake_move();

            if (score > result.score)
            {
                result.score = score;
                result.bestMove = move;

                Board temp = board;

                if (temp.make_move(move))
                {
                    TTEntry &child = transpositionTable[tt_index(temp.zobrist)];

                    // get ponder move from transposition table
                    result.ponderMove = child.bestMove;

                    temp.unmake_move();
                }
            }

            if (score > alpha) alpha = score;
        }

        return result;
    }

    //--------------------------------------------------
    // MAIN SEARCH
    //--------------------------------------------------

    SearchResult find_best_move(Board &board, int maxDepth)
    {
        SearchResult best{};
        best.bestMove = 0;
        best.score = -Eval::INF;
        best.depth = 0;

        stopSearch = false;
        searchStart = std::chrono::steady_clock::now();

        std::memset(killerMoves, 0, sizeof(killerMoves));
        std::memset(historyTable, 0, sizeof(historyTable));

        // iterative deepening
        for (int depth = 1; depth <= maxDepth; depth++)
        {
            // aspiration window setup
            int delta = 20;
            int alpha, beta;

            if (depth == 1)
            {
                alpha = -Eval::INF;
                beta = Eval::INF;
            }
            else
            {
                alpha = best.score - delta;
                beta = best.score + delta;
            }

            SearchResult current;

            // aspiration window re-search
            while (true)
            {
                current = search_root(board, depth, alpha, beta);

                if (stopSearch) break;

                if (current.score <= alpha)
                {
                    alpha -= delta;
                    beta += delta;
                }
                else if (current.score >= beta)
                {
                    beta += delta;
                    alpha -= delta;
                }
                else break;

                delta *= 2;
            }

            if (stopSearch) break;

            best = current;
            best.depth = depth;

            std::cout
                << "info depth " << depth
                << " score cp " << current.score
                << "\n";
        }

        /* Nodes Per Second benchmark
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - searchStart).count();

        std::cout << "nodes: " << nodes
                << " nps: " << nodes * 1000 / elapsed
                << "\n";
        */

        return best;
    }

}
