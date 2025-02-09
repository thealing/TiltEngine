#pragma once

#include "pieces.hpp"
#include "squares.hpp"
#include "scores.hpp"

#include <string.h>

using MoveType = int8_t;

enum : MoveType
{
	MOVE_TYPE_PAWN = PIECE_PAWN,
	MOVE_TYPE_KNIGHT = PIECE_KNIGHT,
	MOVE_TYPE_BISHOP = PIECE_BISHOP,
	MOVE_TYPE_ROOK = PIECE_ROOK,
	MOVE_TYPE_QUEEN = PIECE_QUEEN,
	MOVE_TYPE_KING = PIECE_KING,
	MOVE_TYPE_DOUBLE,
	MOVE_TYPE_EN_PASSANT,
	MOVE_TYPE_PROMOTION_Q,
	MOVE_TYPE_PROMOTION_R,
	MOVE_TYPE_PROMOTION_B,
	MOVE_TYPE_PROMOTION_N,
	MOVE_TYPE_CASTLING_WK,
	MOVE_TYPE_CASTLING_WQ,
	MOVE_TYPE_CASTLING_BK,
	MOVE_TYPE_CASTLING_BQ,
	MOVE_TYPE_COUNT
};

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
		static_assert(sizeof(Move) == sizeof(int32_t));
		return *(int32_t*)this == *(int32_t*)&other;
	}

	inline bool operator!=(const Move& other) const
	{
		return !(*this == other);
	}
};
