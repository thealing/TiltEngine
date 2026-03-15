# Tilt Engine

A high-performance chess engine written in C++ utilizing bitboards and advanced search heuristics.
It supports the standard UCI protocol, allowing it to be used with any common chess GUI.

The playing strength is around 2650 ELO against other engines on CCRL.

## Highlighted Features
- Bitboard move generation
  - Magic bitboards
- Transposition table
  - Zobrist hashing
  - Prefetch
- Iterative deepening
  - Aspiration windows
  - Principal variation
  
## Other Features
- Search
  - Alpha-beta pruning
  - Quiescence search
  - Late move reduction
  - History heuristic
  - Razoring
  - Futility pruning
- Evaluation
  - Tapered evaluation
  - Piece-square tables
  - Mobility scoring
  - Pawn structure analysis
  - Pawn hash-table cache
- UCI protocol support
  - Asynchronous search
  - Perft command
  - Time management
  - Fixed depth
  - Fixed move time
  - Fixed node count
