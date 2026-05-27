# saherator-1.0

## Utils (`utils.h`)

The utils file contains the low-level foundational components used across the entire engine.

This includes:

* Fixed-width integer aliases (`U64`, `U32`, etc.) for cleaner and more explicit bit manipulation.
* Core enums for `Color` and `Piece` representation.
* The default starting FEN string (`STARTFEN`).
* Precomputed bitboard masks for:

  * individual files (`FILE_A` → `FILE_H`)
  * individual ranks (`RANK_1` → `RANK_8`)
* Castling bit masks.
* Move flag bit masks for:

  * captures
  * en passant
  * castling
  * double pawn pushes
  * promotions

### Move Encoding

Moves are packed into a compact 32-bit integer format for faster storage and manipulation during move generation/search.

Layout:

`from | to | piece | captured | flags`

Specifically:

* bits `0-5` → source square
* bits `6-11` → target square
* bits `12-15` → moving piece (or promotion piece)
* bits `16-19` → captured piece
* bits `20-24` → special move flags

For promotions, the stored piece value represents the promoted piece type, while the pawn move itself is implied.

### Bitboard Utilities

Contains helper functions for common bitboard operations:

* `set_bit()` → sets a bit on a square
* `clear_bit()` → clears a bit on a square
* `bit_scan_forward()` → finds least significant bit
* `bit_scan_reverse()` → finds most significant bit
* `pop_lsb()` → removes and returns least significant bit
* `bit_count()` → returns number of set bits

These rely on compiler intrinsics (`__builtin_ctzll`, `__builtin_popcountll`, etc.) for speed.

### Random Number Generation

Includes a lightweight `rand64()` function used for generating 64-bit pseudo-random numbers.

This is primarily intended for things like Zobrist hashing and other hashing-related systems inside the engine.

---

## Board Representation (`board.h`)

The board uses a **hybrid representation**:

* **Bitboards (`pieces[12]`)** → fast move generation and attack calculations
* **Mailbox array (`piece_at[64]`)** → fast direct square lookup
* **Occupancy bitboards (`occ[3]`)** → white occupancy, black occupancy, combined occupancy

This hybrid approach avoids expensive bit scanning when simple square lookups are needed while still keeping bitboard move generation fast.

### Additional board state tracked:

* side to move (`stm`)
* fullmove counter
* halfmove counter
* castling rights
* en passant square
* Zobrist hash
* undo stack
* current ply depth

### Undo System

Moves are fully reversible through an `Undo` stack.

Before each move, the engine stores:

* previous Zobrist hash
* move made
* moved piece
* captured piece
* castling rights
* en passant state
* halfmove clock

This allows fast `make_move()` / `unmake_move()` operations during:

* move generation validation
* search
* perft testing

### FEN Support

Implemented:

* `init_from_fen()` → load arbitrary positions
* `to_fen()` → export current board state
* `reset()` → reset to starting position

### Zobrist Hashing

The board tracks a full Zobrist hash for each position using:

* piece-square hashes
* castling rights hashes
* en passant hashes
* side-to-move hash

This is currently used for transposition tables in search.

### Null move support

`make_null_move()` and `unmake_null_move()` have been implemented for use in null move pruning within search.

---

## Move Generation (`movegen.h`)

Uses bitboards and magic bitboards for fast pseudolegal move generation.

Function `generate_legal(...)` is deprecated due to its expensive use of `make_move(...)` and `unmake_move()` for legality checking. Use `generate_legal_fast(...)` from attacks.h instead.

### Precomputed attack tables

For faster move generation, the engine precomputes:

* pawn pushes
* pawn attacks
* reverse pawn attacks
* knight attacks
* king attacks

This avoids recalculating common attack patterns every move.

The tables need to be initialized by calling `init_movegen()`.

### Magic Bitboards

Sliding pieces use **magic bitboards** for fast attack generation:

* bishops
* rooks
* queens

Attack lookup flow:

1. Mask relevant occupancy bits
2. Multiply by magic number
3. Shift result
4. Lookup attack table entry

This is significantly faster than brute-force ray tracing.

### Incremental Updates

Board occupancies and piece bitboards are updated incrementally during move execution.

This avoids rebuilding board state from scratch after every move.

### Attack Detection

`square_attacked()` checks whether a square is attacked by a given side.

This is heavily used for:

* king safety validation
* castling legality
* legal move generation
* check detection during search

### Legal Move Generation (DEPRECATED)

Perft results match 100%.

Current flow:

`generate_pseudo_legal()`
→ generates all candidate moves

`generate_legal()`
→ validates moves by:

* making the move
* checking king legality
* unmaking the move

---

## Attacks - Move Generation Extension (`attacks.h`)

This module extends the move generation system with **precomputed attack analysis and legality helpers**.

It is used to efficiently detect:

* checks
* pins
* attacked squares
* legal move filtering

### Attack Detection

```cpp
U64 attackers_to(const Board &board, int sq, U64 occ, Color side);
```

Returns a bitboard of all pieces of a given side attacking a square.

### Check Detection

```cpp
bool is_in_check(const Board &board);
```

Returns whether the current side to move is in check.

Internally uses `attackers_to()` on the king square.

### Generation State (`GenState`)

This structure stores **precomputed board state information** used during move generation.

It avoids recomputing expensive bitboard queries repeatedly.

Fields:

* `us / them` → side information
* `kingSq` → king position
* `occ` → full occupancy bitboard
* `usOcc / themOcc` → side occupancies
* `checkers` → pieces currently giving check
* `pinned` → pinned pieces for the side
* `danger` → squares attacked by enemy pieces
* `checkMask` → squares that can block or capture a checker
* `pinRay[64]` → ray from pinned piece through king

### Precomputed Ray Tables

```cpp
extern U64 betweenBB[64][64];
extern U64 lineBB[64][64];
```

Used for fast geometry calculations:

* `betweenBB[from][to]` - squares between two squares
* `lineBB[from][to]` - full ray line through two squares

### Initialization

```cpp
void init_between_tables();
```

Precomputes ray tables at startup.

### Legal Move Generation

#### Full legal move generation

```cpp
void generate_legal_fast(const Board &board, MoveList &list);
```

Generates all legal moves using:

* pinned piece detection
* check masks
* precomputed attack data

Avoids full make/unmake validation by filtering moves upfront.

---

#### Capture-only generation

```cpp
void generate_captures(const Board& board, MoveList& list);
```

Generates only legal captures.

Used mainly in quiescence search.

---

## UCI Support (`uci.h`)

The šaherator engine supports the Universal Chess Interface protocol, which allows it to communicate with GUI clients such as Arena Chess GUI, Cute Chess, or Lichess bot integrations.

Currently implemented commands:

* `uci`
* `isready`
* `ucinewgame`
* `position (startpos / fen ...) (moves ...)`
* `go (depth / movetime / time control / infinite / ponder)`
* `ponderhit`
* `stop`
* `quit`

There is also a small debug command:

* `d` → prints the current board state

### Position Parsing

The engine supports both:

`position startpos`

and

`position fen ...`

After loading the position, it can also parse move sequences:

`position startpos moves e2e4 e7e5 g1f3`

Moves are validated by generating all legal moves and matching them against UCI notation.

This ensures that illegal moves are rejected rather than corrupting board state.

### Move Conversion

`move_to_string()` converts internal move encoding into standard UCI notation:

Example:

* `e2e4`
* `e7e8q` (promotion)

### Search Integration

Search runs in a separate worker thread while UCI communication remains responsive on the main thread.

When the GUI sends:

`go`

the engine calls:

`Search::find_best_move()`

and returns:

`bestmove <move>`

This connects the move generator, evaluation function, and search system, allowing the engine to play actual games.

---

## Evaluation (`eval.h`)

The evaluation function uses a **tapered middlegame/endgame system** combined with **piece-square tables and positional heuristics**.

The goal is to produce a fast static evaluation that works well with alpha-beta search.

### Material + Piece-Square Tables (PSQT)

Each piece has:

* a base material value (`mgValue`, `egValue`)
* a positional bonus table (PeSTO-style)

The engine uses **separate middlegame and endgame tables**, then blends them based on game phase.

PSQT tables are:

* pawn
* knight
* bishop
* rook
* queen
* king

### Game Phase System

The engine tracks game phase dynamically based on piece composition:

Each piece contributes:

```cpp
pawn   = 0
knight = 1
bishop = 1
rook   = 2
queen  = 4
king   = 0
```

Phase is clamped to:

```cpp
MAX_PHASE = 24
```

This is used for tapered evaluation between middlegame and endgame.

### Tapered Evaluation

Final score is computed as:

```
score = (mg * phase + eg * (MAX_PHASE - phase)) / MAX_PHASE
```

This allows the engine to smoothly transition between:

* middlegame evaluation (king safety, activity)
* endgame evaluation (pawn advancement, king activity)

### Pawn Structure

Pawn evaluation includes:

#### Doubled pawns

Penalizes multiple pawns on the same file:

* stronger penalty in middlegame
* weaker in endgame

#### Isolated pawns

A pawn is considered isolated if no friendly pawns exist on adjacent files.

Penalty is applied per pawn.

#### Passed pawns

A pawn is considered passed if no enemy pawns exist in its forward “passed mask”.

Bonus increases as pawn advances:

* small early bonus
* very large bonus near promotion

### Pawn Masks

The engine precomputes **passed pawn masks** for each square:

* squares in front of a pawn
* adjacent files included
* used for fast passed pawn detection

This avoids recomputing pawn structure dynamically during evaluation.

### Rook Activity

Rooks are rewarded for file activity:

#### Open file

No pawns on the file:

* highest bonus

#### Semi-open file

Only enemy pawns present:

* smaller bonus

This encourages rooks to occupy active lines.

### Bishop Pair

If a side has two bishops:

* bonus in middlegame
* slightly larger bonus in endgame

This reflects long-term diagonal dominance.

### King Safety

King safety is evaluated using two components:

#### Pawn shield

Counts friendly pawns in front of the king (within a small forward zone).

Result:

* no pawns → heavy penalty
* 1 pawn → moderate penalty
* 2+ pawns → reduced or negative penalty

#### Enemy proximity pressure

Enemy pieces close to the king contribute attack pressure:

* distance is measured using Manhattan distance
* only pieces within radius 3 contribute
* each piece type has a weight:

```cpp
pawn   = 0
knight = 1
bishop = 1
rook   = 2
queen  = 4
```

This approximates attacking pressure around the king.

### Evaluation Flow

The evaluation runs in this order:

1. Piece loop (material + PSQT + phase)
2. Pawn structure evaluation
3. Passed pawn detection
4. Rook file bonuses
5. Bishop pair bonus
6. King safety
7. Final tapered blend
8. Side-to-move adjustment

### Side to Move Adjustment

Final score is flipped depending on side to move:

```cpp
return (board.stm == WHITE)
    ? score
    : -score;
```

This ensures evaluation is always from the perspective of the side to move.

### Future Improvements

Planned upgrades include:

* mobility bonuses
* stronger pawn structure evaluation
* better king safety heuristics / king endgame activity

---

## Search (`search.h`)

The search module is responsible for exploring the game tree and finding the best move within a given time or depth constraint.

It is built around a **Negamax alpha-beta framework**, extended with multiple modern pruning and ordering techniques:

* transposition table acceleration
* aggressive move ordering
* pruning (null move, LMR)
* quiescence stabilization
* iterative deepening with aspiration windows

This allows for **fast cutoff-heavy search behavior while maintaining tactical stability**.

### Core Search Architecture

Main entry point:

```cpp
find_best_move()
```

Search progressively increases depth until time runs out, always returning the best completed iteration.

### Negamax Search

```cpp
negamax(Board &board, int depth, int alpha, int beta)
```

Core recursive function of the engine.

Features:

* alpha-beta pruning
* transposition table cuts
* mate distance pruning
* null move pruning
* check extensions
* late move reductions (LMR)
* principal variation search (PVS-style re-search)
* repetition + 50-move rule detection

Mate scores are adjusted by ply to prefer faster mates.

### Quiescence Search

```cpp
quiescence(Board &board, int alpha, int beta)
```

Used when depth reaches zero.

Only evaluates **capture moves**, preventing horizon effect.

Includes:

* stand-pat evaluation
* delta pruning
* TT cutoffs
* capture ordering (MVV-LVA)

### Transposition Table

Fixed-size hash table indexed by Zobrist key:

Stores:

* position key
* best move
* score
* search depth
* node flag (exact / alpha / beta)

Used for:

* pruning repeated positions
* move ordering
* improving cutoff rates

### Move Ordering

Moves are ordered before search to maximize pruning efficiency.

Priority:

1. transposition table move
2. captures (MVV-LVA)
3. killer moves
4. history heuristic

Killer moves are stored per ply.

History heuristic rewards quiet moves that cause beta cutoffs.

### Null Move Pruning

Applied in non-check positions with sufficient material:

If a reduced-depth search after a null move still fails high, the position is cut off.

Used to detect overwhelmingly strong positions early.

### Late Move Reductions (LMR)

Moves searched later in move ordering are searched at reduced depth.

Conditions:

* not a capture or promotion
* not in check
* late in move list
* sufficient depth

If reduced search improves alpha, full re-search is performed.

### Check Extensions

If the side to move is in check, depth is increased by 1.

This ensures forced lines are searched more accurately.

### Aspiration Windows

At root, search starts with a narrow window around previous score:

* if score is outside window - re-search with wider bounds
* otherwise accept result

This improves efficiency in stable positions.

### Repetition & Draw Detection

Search terminates branches early if:

* 3-fold repetition is detected (via Zobrist history)
* 50-move rule is reached

### Mate Handling

Mate scores are stored as large bounded values.

They are adjusted by `board.ply` so that faster mates are preferred.

### Time Management Integration

Search is time-controlled via:

* `allocatedTimeMs`
* `stopSearch` flag
* periodic time checks (every ~2048 nodes)

Search stops safely at any depth and returns the best completed iteration.

Pondering mode disables time cutoff checks.

### Iterative Deepening

Search runs depth by depth:

```
depth 1 → quick best guess
depth 2 → refinement
depth 3 → refinement
...
```

Each iteration improves move ordering for the next one.

### Root Search

```cpp
search_root()
```

Handles:

* full move list generation
* ordering using TT move
* evaluating each root move via negamax
* extracting ponder move from TT

### History Heuristic

Quiet moves that cause beta cutoffs are rewarded:

* scaled by depth²
* normalized to prevent overflow
* stored per from-to square pair

This improves long-term move ordering quality.

### Killer Moves

Two killer moves are stored per ply:

* non-capture moves that cause beta cutoffs
* prioritized in future searches at same depth

### Future Improvements

Current search still lacks:

* singular extensions
* advanced pruning (razoring / futility pruning)
* improved SEE-based pruning
* opening books
* endgame tablebase support
* SMP parallel search
* neural evaluation integration

---

## Time Management (`timeman.h`)

This module handles how much time the engine should spend per move based on UCI time controls.

UCI gives time --> timeman allocates budget --> iterative deepening search runs -->
constantly checks time --> stops safely --> returns best completed depth result.

### TimeControl

```cpp
struct TimeControl {...};
```

Represents time data from UCI `go` command.

### Time Allocation

```cpp
int allocate_time(const Board& board, const TimeControl& tc);
```

Returns how many milliseconds the engine should think for the current move.

This function:

* Uses remaining clock time (`wtime / btime`)
* Adds a portion of increment (`winc / binc`)
* Adjusts based on game phase:

  * opening → more conservative
  * middlegame → most time usage
  * endgame → slightly faster moves
* Keeps a safety reserve so the engine doesn’t flag

If `movetime` is set, it overrides everything.

---
