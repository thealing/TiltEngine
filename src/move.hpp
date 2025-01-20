#pragma once

#include "pieces.hpp"
#include "squares.hpp"
#include "scores.hpp"

#include <string.h>

using MoveType = int8_t;

inline constexpr MoveType MOVE_TYPE_PAWN = 0;
inline constexpr MoveType MOVE_TYPE_KNIGHT = 1;
inline constexpr MoveType MOVE_TYPE_BISHOP = 2;
inline constexpr MoveType MOVE_TYPE_ROOK = 3;
inline constexpr MoveType MOVE_TYPE_QUEEN = 4;
inline constexpr MoveType MOVE_TYPE_KING = 5;
inline constexpr MoveType MOVE_TYPE_DOUBLE = 6;
inline constexpr MoveType MOVE_TYPE_EN_PASSANT = 7;
inline constexpr MoveType MOVE_TYPE_PROMOTION_Q = 8;
inline constexpr MoveType MOVE_TYPE_PROMOTION_R = 9;
inline constexpr MoveType MOVE_TYPE_PROMOTION_B = 10;
inline constexpr MoveType MOVE_TYPE_PROMOTION_N = 11;
inline constexpr MoveType MOVE_TYPE_CASTLING_WK = 12;
inline constexpr MoveType MOVE_TYPE_CASTLING_WQ = 13;
inline constexpr MoveType MOVE_TYPE_CASTLING_BK = 14;
inline constexpr MoveType MOVE_TYPE_CASTLING_BQ = 15;
inline constexpr MoveType MOVE_TYPE_COUNT = 16;

struct Move
{
	Square src_square;
	Square dst_square;
	MoveType type;
	Piece captured_piece;

	explicit inline operator uint16_t()
	{
		return (uint16_t)src_square | (uint16_t)dst_square << 6 | (uint16_t)type << 12;
	}

	inline Piece get_moved_piece() const
	{
		switch (type)
		{
			case MOVE_TYPE_DOUBLE:
			case MOVE_TYPE_EN_PASSANT:
			case MOVE_TYPE_PROMOTION_Q:
			case MOVE_TYPE_PROMOTION_R:
			case MOVE_TYPE_PROMOTION_B:
			case MOVE_TYPE_PROMOTION_N:
				return PIECE_PAWN;
			case MOVE_TYPE_CASTLING_WK:
			case MOVE_TYPE_CASTLING_WQ:
			case MOVE_TYPE_CASTLING_BK:
			case MOVE_TYPE_CASTLING_BQ:
				return PIECE_KING;
			default:
				return type;
		}
	}

	inline bool is_promotion() const
	{
		switch (type)
		{
			case MOVE_TYPE_PROMOTION_Q:
			case MOVE_TYPE_PROMOTION_R:
			case MOVE_TYPE_PROMOTION_B:
			case MOVE_TYPE_PROMOTION_N:
				return true;
			default:
				return false;
		} 
	}

	inline bool operator==(const Move& other) const
	{
		return memcmp(this, &other, sizeof(Move)) == 0;
	}

	inline bool operator!=(const Move& other) const
	{
		return !(*this == other);
	}
};
