#pragma once

#include <cstdint>

using Rank = int8_t;
using File = int8_t;
using Square = int8_t;

inline constexpr Rank RANK_8 = 0;
inline constexpr Rank RANK_7 = 1;
inline constexpr Rank RANK_6 = 2;
inline constexpr Rank RANK_5 = 3;
inline constexpr Rank RANK_4 = 4;
inline constexpr Rank RANK_3 = 5;
inline constexpr Rank RANK_2 = 6;
inline constexpr Rank RANK_1 = 7;
inline constexpr Rank RANK_COUNT = 8;

inline constexpr File FILE_A = 0;
inline constexpr File FILE_B = 1;
inline constexpr File FILE_C = 2;
inline constexpr File FILE_D = 3;
inline constexpr File FILE_E = 4;
inline constexpr File FILE_F = 5;
inline constexpr File FILE_G = 6;
inline constexpr File FILE_H = 7;
inline constexpr File FILE_COUNT = 8;

inline constexpr Square SQUARE_NONE = -1;
inline constexpr Square SQUARE_A8 = 0;
inline constexpr Square SQUARE_B8 = 1;
inline constexpr Square SQUARE_C8 = 2;
inline constexpr Square SQUARE_D8 = 3;
inline constexpr Square SQUARE_E8 = 4;
inline constexpr Square SQUARE_F8 = 5;
inline constexpr Square SQUARE_G8 = 6;
inline constexpr Square SQUARE_H8 = 7;
inline constexpr Square SQUARE_A7 = 8;
inline constexpr Square SQUARE_B7 = 9;
inline constexpr Square SQUARE_C7 = 10;
inline constexpr Square SQUARE_D7 = 11;
inline constexpr Square SQUARE_E7 = 12;
inline constexpr Square SQUARE_F7 = 13;
inline constexpr Square SQUARE_G7 = 14;
inline constexpr Square SQUARE_H7 = 15;
inline constexpr Square SQUARE_A6 = 16;
inline constexpr Square SQUARE_B6 = 17;
inline constexpr Square SQUARE_C6 = 18;
inline constexpr Square SQUARE_D6 = 19;
inline constexpr Square SQUARE_E6 = 20;
inline constexpr Square SQUARE_F6 = 21;
inline constexpr Square SQUARE_G6 = 22;
inline constexpr Square SQUARE_H6 = 23;
inline constexpr Square SQUARE_A5 = 24;
inline constexpr Square SQUARE_B5 = 25;
inline constexpr Square SQUARE_C5 = 26;
inline constexpr Square SQUARE_D5 = 27;
inline constexpr Square SQUARE_E5 = 28;
inline constexpr Square SQUARE_F5 = 29;
inline constexpr Square SQUARE_G5 = 30;
inline constexpr Square SQUARE_H5 = 31;
inline constexpr Square SQUARE_A4 = 32;
inline constexpr Square SQUARE_B4 = 33;
inline constexpr Square SQUARE_C4 = 34;
inline constexpr Square SQUARE_D4 = 35;
inline constexpr Square SQUARE_E4 = 36;
inline constexpr Square SQUARE_F4 = 37;
inline constexpr Square SQUARE_G4 = 38;
inline constexpr Square SQUARE_H4 = 39;
inline constexpr Square SQUARE_A3 = 40;
inline constexpr Square SQUARE_B3 = 41;
inline constexpr Square SQUARE_C3 = 42;
inline constexpr Square SQUARE_D3 = 43;
inline constexpr Square SQUARE_E3 = 44;
inline constexpr Square SQUARE_F3 = 45;
inline constexpr Square SQUARE_G3 = 46;
inline constexpr Square SQUARE_H3 = 47;
inline constexpr Square SQUARE_A2 = 48;
inline constexpr Square SQUARE_B2 = 49;
inline constexpr Square SQUARE_C2 = 50;
inline constexpr Square SQUARE_D2 = 51;
inline constexpr Square SQUARE_E2 = 52;
inline constexpr Square SQUARE_F2 = 53;
inline constexpr Square SQUARE_G2 = 54;
inline constexpr Square SQUARE_H2 = 55;
inline constexpr Square SQUARE_A1 = 56;
inline constexpr Square SQUARE_B1 = 57;
inline constexpr Square SQUARE_C1 = 58;
inline constexpr Square SQUARE_D1 = 59;
inline constexpr Square SQUARE_E1 = 60;
inline constexpr Square SQUARE_F1 = 61;
inline constexpr Square SQUARE_G1 = 62;
inline constexpr Square SQUARE_H1 = 63;
inline constexpr Square SQUARE_COUNT = 64;

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
