#pragma once

#include "pieces.hpp"
#include "bitboards.hpp"

template<Color color>
struct MoveHelper
{
	static constexpr Rank get_promotion_rank();

	static constexpr Rank get_starting_rank();

	template<int Amount>
	static constexpr Square move_forward(Square square);

	template<int Amount>
	static constexpr Square move_backward(Square square);

	static constexpr Square move_backward_left(Square square);

	static constexpr Square move_backward_right(Square square);

	template<int Amount>
	static constexpr Bitboard shift_forward(Bitboard bitboard);

	template<int Amount>
	static constexpr Bitboard shift_backward(Bitboard bitboard);

	static constexpr Bitboard shift_forward_left(Bitboard bitboard);

	static constexpr Bitboard shift_forward_right(Bitboard bitboard);
};

template<>
struct MoveHelper<COLOR_WHITE>
{
	static constexpr Rank get_promotion_rank()
	{
		return RANK_8;
	}

	static constexpr Rank get_starting_rank()
	{
		return RANK_2;
	}

	template<int Amount>
	static constexpr Square move_forward(Square square)
	{
		return square - 8 * Amount;
	}

	template<int Amount>
	static constexpr Square move_backward(Square square)
	{
		return square + 8 * Amount;
	}

	static constexpr Square move_backward_left(Square square)
	{
		return square + 7;
	}

	static constexpr Square move_backward_right(Square square)
	{
		return square + 9;
	}

	template<int Amount>
	static constexpr Bitboard shift_forward(Bitboard bitboard)
	{
		return bitboard >> 8 * Amount;
	}

	template<int Amount>
	static constexpr Bitboard shift_backward(Bitboard bitboard)
	{
		return bitboard << 8 * Amount;
	}

	static constexpr Bitboard shift_forward_left(Bitboard bitboard)
	{
		return bitboard >> 9;
	}

	static constexpr Bitboard shift_forward_right(Bitboard bitboard)
	{
		return bitboard >> 7;
	}
};

template<>
struct MoveHelper<COLOR_BLACK>
{
	static constexpr Rank get_promotion_rank()
	{
		return RANK_1;
	}

	static constexpr Rank get_starting_rank()
	{
		return RANK_7;
	}

	template<int Amount>
	static constexpr Square move_forward(Square square)
	{
		return square + 8 * Amount;
	}

	template<int Amount>
	static constexpr Square move_backward(Square square)
	{
		return square - 8 * Amount;
	}

	static constexpr Square move_backward_left(Square square)
	{
		return square - 9;
	}

	static constexpr Square move_backward_right(Square square)
	{
		return square - 7;
	}

	template<int Amount>
	static constexpr Bitboard shift_forward(Bitboard bitboard)
	{
		return bitboard << 8 * Amount;
	}

	template<int Amount>
	static constexpr Bitboard shift_backward(Bitboard bitboard)
	{
		return bitboard >> 8 * Amount;
	}

	static constexpr Bitboard shift_forward_left(Bitboard bitboard)
	{
		return bitboard << 7;
	}

	static constexpr Bitboard shift_forward_right(Bitboard bitboard)
	{
		return bitboard << 9;
	}
};

template<Color color>
inline constexpr Rank get_promotion_rank()
{
	return MoveHelper<color>::get_promotion_rank();
}

template<Color color>
inline constexpr Rank get_starting_rank()
{
	return MoveHelper<color>::get_starting_rank();
}

template<Color color, int Amount = 1>
inline constexpr Square move_forward(Square square)
{
	return MoveHelper<color>::template move_forward<Amount>(square);
}

template<Color color, int Amount = 1>
inline constexpr Square move_backward(Square square)
{
	return MoveHelper<color>::template move_backward<Amount>(square);
}

template<Color color>
inline constexpr Square move_backward_left(Square square)
{
	return MoveHelper<color>::move_backward_left(square);
}

template<Color color>
inline constexpr Square move_backward_right(Square square)
{
	return MoveHelper<color>::move_backward_right(square);
}

template<Color color, int Amount = 1>
inline constexpr Bitboard shift_forward(Bitboard bitboard)
{
	return MoveHelper<color>::template shift_forward<Amount>(bitboard);
}

template<Color color, int Amount = 1>
inline constexpr Bitboard shift_backward(Bitboard bitboard)
{
	return MoveHelper<color>::template shift_backward<Amount>(bitboard);
}

template<Color color>
inline constexpr Bitboard shift_forward_left(Bitboard bitboard)
{
	return MoveHelper<color>::shift_forward_left(bitboard) & ~get_file_mask(FILE_H);
}

template<Color color>
inline constexpr Bitboard shift_forward_right(Bitboard bitboard)
{
	return MoveHelper<color>::shift_forward_right(bitboard) & ~get_file_mask(FILE_A);
}

inline constexpr Bitboard shift_left(Bitboard bitboard)
{
	return (bitboard >> 1) & ~get_file_mask(FILE_H);
}

inline constexpr Bitboard shift_right(Bitboard bitboard)
{
	return (bitboard << 1) & ~get_file_mask(FILE_A);
}

template<Color color>
inline constexpr Bitboard span_forward(Bitboard bitboard)
{
	bitboard |= shift_forward<color, 1>(bitboard);
	bitboard |= shift_forward<color, 2>(bitboard);
	bitboard |= shift_forward<color, 4>(bitboard);
	return bitboard;
}

template<Color color>
inline constexpr Bitboard span_backward(Bitboard bitboard)
{
	bitboard |= shift_backward<color, 1>(bitboard);
	bitboard |= shift_backward<color, 2>(bitboard);
	bitboard |= shift_backward<color, 4>(bitboard);
	return bitboard;
}
