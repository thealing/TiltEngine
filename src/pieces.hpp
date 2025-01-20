#pragma once

#include <stdint.h>

using Piece = int8_t;
using Color = int8_t;

inline constexpr Piece PIECE_NONE = -1;
inline constexpr Piece PIECE_PAWN = 0;
inline constexpr Piece PIECE_KNIGHT = 1;
inline constexpr Piece PIECE_BISHOP = 2;
inline constexpr Piece PIECE_ROOK = 3;
inline constexpr Piece PIECE_QUEEN = 4;
inline constexpr Piece PIECE_KING = 5;
inline constexpr Piece PIECE_COUNT = 6;

inline constexpr Color COLOR_WHITE = 0;
inline constexpr Color COLOR_BLACK = 1;
inline constexpr Color COLOR_COUNT = 2;

inline constexpr Color flip_color(Color color)
{
	return color ^ 1;
}
