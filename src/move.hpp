#pragma once

#include "types.hpp"

#include <memory.h>

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
	MOVE_TYPE_COUNT,
};

struct Move
{
	Square src_square;
	Square dst_square;
	MoveType type;
	Piece captured_piece;
};

inline std::ostream& operator<<(std::ostream& os, const Move& move)
{
	format_square(os, move.src_square);
	format_square(os, move.dst_square);
	switch (move.type) {
		case MOVE_TYPE_PROMOTION_Q:
			return os << 'q';
		case MOVE_TYPE_PROMOTION_R:
			return os << 'r';
		case MOVE_TYPE_PROMOTION_B:
			return os << 'b';
		case MOVE_TYPE_PROMOTION_N:
			return os << 'n';
		default:
			return os;
	}
}
