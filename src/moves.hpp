#pragma once

#include "bitboards.hpp"

template<Color color>
inline constexpr Rank get_promotion_rank();

template<>
inline constexpr Rank get_promotion_rank<COLOR_WHITE>()
{
	return RANK_8;
}

template<>
inline constexpr Rank get_promotion_rank<COLOR_BLACK>()
{
	return RANK_1;
}

template<Color color>
inline constexpr Rank get_starting_rank();

template<>
inline constexpr Rank get_starting_rank<COLOR_WHITE>()
{
	return RANK_2;
}

template<>
inline constexpr Rank get_starting_rank<COLOR_BLACK>()
{
	return RANK_7;
}

template<Color color>
inline constexpr Square move_backward(Square square);

template<>
inline constexpr Square move_backward<COLOR_WHITE>(Square square)
{
	return square + 8;
}

template<>
inline constexpr Square move_backward<COLOR_BLACK>(Square square)
{
	return square - 8;
}

template<Color color>
inline constexpr Square move_backward_left(Square square);

template<>
inline constexpr Square move_backward_left<COLOR_WHITE>(Square square)
{
	return square + 7;
}

template<>
inline constexpr Square move_backward_left<COLOR_BLACK>(Square square)
{
	return square - 9;
}

template<Color color>
inline constexpr Square move_backward_right(Square square);

template<>
inline constexpr Square move_backward_right<COLOR_WHITE>(Square square)
{
	return square + 9;
}

template<>
inline constexpr Square move_backward_right<COLOR_BLACK>(Square square)
{
	return square - 7;
}

template<Color color>
inline constexpr Bitboard shift_forward(Bitboard bitboard);

template<>
inline constexpr Bitboard shift_forward<COLOR_WHITE>(Bitboard bitboard)
{
	return bitboard >> 8;
}

template<>
inline constexpr Bitboard shift_forward<COLOR_BLACK>(Bitboard bitboard)
{
	return bitboard << 8;
}

template<Color color>
inline constexpr Bitboard shift_backward(Bitboard bitboard);

template<>
inline constexpr Bitboard shift_backward<COLOR_WHITE>(Bitboard bitboard)
{
	return bitboard << 8;
}

template<>
inline constexpr Bitboard shift_backward<COLOR_BLACK>(Bitboard bitboard)
{
	return bitboard >> 8;
}

template<Color color>
inline constexpr Bitboard shift_forward_left(Bitboard bitboard);

template<>
inline constexpr Bitboard shift_forward_left<COLOR_WHITE>(Bitboard bitboard)
{
	return bitboard >> 9;
}

template<>
inline constexpr Bitboard shift_forward_left<COLOR_BLACK>(Bitboard bitboard)
{
	return bitboard << 7;
}

template<Color color>
inline constexpr Bitboard shift_forward_right(Bitboard bitboard);

template<>
inline constexpr Bitboard shift_forward_right<COLOR_WHITE>(Bitboard bitboard)
{
	return bitboard >> 7;
}

template<>
inline constexpr Bitboard shift_forward_right<COLOR_BLACK>(Bitboard bitboard)
{
	return bitboard << 9;
}
