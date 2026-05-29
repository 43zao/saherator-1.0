# Šaherator

A bitboard-based chess engine written from scratch in C++.

Built with passion as a learning project that connects my career with my favorite passtime. :]

2000+ Lichess elo.

Challenge it at: https://lichess.org/@/saherator

## Features

- Fast legal / pseudo-legal move generation
- Captures only generation
- Magic bitboards
- Flawless perft
- UCI support
- Alpha-beta pruning negamax search
- Move ordering
- Killer moves
- History heuristics
- Principal variation search
- Late move reductions
- Check extensions
- Null move pruning
- Quiescence search
- Transposition tables
- Aspiration windows
- Positional evaluation:
  - material
  - piece square tables
  - king safety
  - pawn structure
  - tapered evaluation (mid / endgame)
- Iterative deepening
- Time management
- Pondering

## Roadmap

- Add Build instructions for the engine
- Multithreading options for search
- Static exchange evaluation (SEE)
- Quiescence improvements
- Incremental cached evaluations
- Adding perft to UCI
- Adding namespaces to board, movegen, attacks
- Recapture, passed pawn extensions
- Aspiration window tuning
- History heuristic decay
- Transposition table upgrades
- Mobility evaluation
- King safety eval improvements
- Pawn structure eval improvements
- Opening books implementation
- Endgame tablebase support

## Documentation

See `docs/`
