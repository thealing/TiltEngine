#pragma once

#include <stdint.h>

using Rank = int8_t;
using File = int8_t;
using Square = int8_t;

enum : Rank
{ 
	RANK_8, 
	RANK_7, 
	RANK_6, 
	RANK_5, 
	RANK_4, 
	RANK_3, 
	RANK_2, 
	RANK_1, 
	RANK_COUNT 
};

enum : File
{ 
	FILE_A, 
	FILE_B, 
	FILE_C, 
	FILE_D, 
	FILE_E, 
	FILE_F, 
	FILE_G, 
	FILE_H, 
	FILE_COUNT 
};

enum : Square
{ 
	SQUARE_NONE = -1,
	SQUARE_A8, SQUARE_B8, SQUARE_C8, SQUARE_D8, SQUARE_E8, SQUARE_F8, SQUARE_G8, SQUARE_H8,
	SQUARE_A7, SQUARE_B7, SQUARE_C7, SQUARE_D7, SQUARE_E7, SQUARE_F7, SQUARE_G7, SQUARE_H7,
	SQUARE_A6, SQUARE_B6, SQUARE_C6, SQUARE_D6, SQUARE_E6, SQUARE_F6, SQUARE_G6, SQUARE_H6,
	SQUARE_A5, SQUARE_B5, SQUARE_C5, SQUARE_D5, SQUARE_E5, SQUARE_F5, SQUARE_G5, SQUARE_H5,
	SQUARE_A4, SQUARE_B4, SQUARE_C4, SQUARE_D4, SQUARE_E4, SQUARE_F4, SQUARE_G4, SQUARE_H4,
	SQUARE_A3, SQUARE_B3, SQUARE_C3, SQUARE_D3, SQUARE_E3, SQUARE_F3, SQUARE_G3, SQUARE_H3,
	SQUARE_A2, SQUARE_B2, SQUARE_C2, SQUARE_D2, SQUARE_E2, SQUARE_F2, SQUARE_G2, SQUARE_H2,
	SQUARE_A1, SQUARE_B1, SQUARE_C1, SQUARE_D1, SQUARE_E1, SQUARE_F1, SQUARE_G1, SQUARE_H1,
	SQUARE_COUNT 
};

inline constexpr Square make_square(Rank rank, File file)
{
	return Square(rank * 8 + file);
}

inline constexpr Rank get_square_rank(Square square)
{
	return square / 8;
}

inline constexpr File get_square_file(Square square)
{
	return square % 8;
}

inline constexpr Square mirror_rank(Square square)
{
	return square ^ 56;
}

inline constexpr Square mirror_file(Square square)
{
	return square ^ 7;
}
