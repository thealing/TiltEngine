#pragma once

#include <stdint.h>

using Piece = int8_t;
using Color = int8_t;

enum : Piece 
{ 
	PIECE_NONE = -1,
	PIECE_PAWN,
	PIECE_KNIGHT,
	PIECE_BISHOP,
	PIECE_ROOK,
	PIECE_QUEEN,
	PIECE_KING,
	PIECE_COUNT
};

enum : Color 
{ 
	COLOR_WHITE,
	COLOR_BLACK,
	COLOR_COUNT
};

inline constexpr Color flip_color(Color color)
{
	return color ^ 1;
}
