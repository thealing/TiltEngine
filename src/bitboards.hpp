#pragma once

#include "squares.hpp"

#include <intrin.h>

using Bitboard = uint64_t;

inline constexpr Bitboard get_square_mask(Square square)
{
	return 1ULL << square;
}

template<typename... Squares>
inline constexpr Bitboard get_square_mask(Square square, Squares... squares)
{
	return get_square_mask(square) | get_square_mask(squares...);
}

inline constexpr Bitboard get_rank_mask(Rank rank)
{
	return 0x00000000000000FFULL << rank * 8;
}

inline constexpr Bitboard get_file_mask(File file)
{
	return 0x0101010101010101ULL << file;
}

inline constexpr bool test_square(Bitboard bitboard, Square square)
{
	return bitboard & get_square_mask(square);
}

inline constexpr void set_square(Bitboard& bitboard, Square square)
{
	bitboard |= get_square_mask(square);
}

inline constexpr void reset_square(Bitboard& bitboard, Square square)
{
	bitboard &= ~get_square_mask(square);
}

inline Square get_square(Bitboard bitboard)
{
#ifdef _MSC_VER
	return (Square)_tzcnt_u64(bitboard);
#else
	return (Square)__builtin_ctzll(bitboard);
#endif
}

inline Square pop_square(Bitboard& bitboard)
{
	Square square = get_square(bitboard);
	reset_square(bitboard, square);
	return square;
}

inline int count_squares(Bitboard bitboard)
{
#ifdef _MSC_VER
	return (int)__popcnt64(bitboard);
#else
	return __builtin_popcountll(bitboard);
#endif
}

inline void flip_squares(Bitboard& bitboard)
{
#ifdef _MSC_VER
	bitboard = _byteswap_uint64(bitboard);
#else
	bitboard = __builtin_bswap64(bitboard);
#endif
}
