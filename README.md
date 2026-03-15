# Tilt Engine

A high-performance chess engine written in C++ utilizing bitboards and advanced search heuristics.
It supports the standard UCI protocol, allowing it to be used with any common chess GUI.

The playing strength is around 2650 ELO against other engines on CCRL.

## Highlighted Features

- Bitboard move generation
- Tapered evaluation
- Transposition table
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
  - Piece-square tables
  - Mobility scoring
  - Pawn structure analysis
  - Pawn hash-table cache
- UCI protocol support
  - Time management
  - Fixed move time
  - Fixed depth search
  - Fixed node search
- Technical features
  - Zobrist hashing
  - Magic bitboards
  - Asynchronous search
  - Perft support
