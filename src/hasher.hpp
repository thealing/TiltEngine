#pragma once

#include "position.hpp"
#include "hash.hpp"
#include "random.hpp"

class Hasher
{
public:
	inline constexpr Hash get_position_hash(const Position& position) const
	{
		Hash hash = 0;
		for (Color color = 0; color < COLOR_COUNT; color++)
		{
			for (Piece piece = 0; piece < PIECE_COUNT; piece++)
			{
				Bitboard mask = position.colors[color] & position.pieces[piece];
				while (mask != 0)
				{
					Square square = pop_square(mask);
					hash ^= _square_piece_hashes[color][square][piece];
				}
			}
		}
		hash ^= (Hash)position.castling_mask;
		hash ^= (Hash)position.current_color;
		hash ^= (Hash)position.en_passant_square;
		return hash;
	}

	template<Color color>
	inline constexpr Hash get_move_hash(const Move& move, const Position& old_position, const Position& new_position) const
	{
		constexpr Color opponent_color = flip_color(color);
		Hash hash = 0;
		switch (move.type)
		{
			case MOVE_TYPE_DOUBLE:
				hash ^= _square_piece_hashes[color][move.src_square][PIECE_PAWN];
				hash ^= _square_piece_hashes[color][move.dst_square][PIECE_PAWN];
				break;
			case MOVE_TYPE_EN_PASSANT:
				hash ^= _square_piece_hashes[color][move.src_square][PIECE_PAWN];
				hash ^= _square_piece_hashes[color][move.dst_square][PIECE_PAWN];
				hash ^= _square_piece_hashes[opponent_color][move_backward<color>(move.dst_square)][PIECE_PAWN];
				break;
			case MOVE_TYPE_PROMOTION_Q:
				hash ^= _square_piece_hashes[color][move.src_square][PIECE_PAWN];
				hash ^= _square_piece_hashes[color][move.dst_square][PIECE_QUEEN];
				break;
			case MOVE_TYPE_PROMOTION_R:
				hash ^= _square_piece_hashes[color][move.src_square][PIECE_PAWN];
				hash ^= _square_piece_hashes[color][move.dst_square][PIECE_ROOK];
				break;
			case MOVE_TYPE_PROMOTION_B:
				hash ^= _square_piece_hashes[color][move.src_square][PIECE_PAWN];
				hash ^= _square_piece_hashes[color][move.dst_square][PIECE_BISHOP];
				break;
			case MOVE_TYPE_PROMOTION_N:
				hash ^= _square_piece_hashes[color][move.src_square][PIECE_PAWN];
				hash ^= _square_piece_hashes[color][move.dst_square][PIECE_KNIGHT];
				break;
			case MOVE_TYPE_CASTLING_WK:
				hash ^= _castling_hash_wk;
				break;
			case MOVE_TYPE_CASTLING_WQ:
				hash ^= _castling_hash_wq;
				break;
			case MOVE_TYPE_CASTLING_BK:
				hash ^= _castling_hash_bk;
				break;
			case MOVE_TYPE_CASTLING_BQ:
				hash ^= _castling_hash_bq;
				break;
			default:
				hash ^= _square_piece_hashes[color][move.src_square][move.type];
				hash ^= _square_piece_hashes[color][move.dst_square][move.type];
				break;
		}
		if (move.captured_piece != PIECE_NONE)
		{
			hash ^= _square_piece_hashes[opponent_color][move.dst_square][move.captured_piece];
		}
		hash ^= (Hash)old_position.castling_mask;
		hash ^= (Hash)old_position.current_color;
		hash ^= (Hash)old_position.en_passant_square;
		hash ^= (Hash)new_position.castling_mask;
		hash ^= (Hash)new_position.current_color;
		hash ^= (Hash)new_position.en_passant_square;
		return hash;
	}

	inline constexpr Hasher()
	{
		Random random;
		for (Color color = 0; color < COLOR_COUNT; color++)
		{
			for (Square square = 0; square < SQUARE_COUNT; square++)
			{
				for (Piece piece = 0; piece < PIECE_COUNT; piece++)
				{
					_square_piece_hashes[color][square][piece] = random.next();
				}
			}
		}
		_castling_hash_wk ^= _square_piece_hashes[COLOR_WHITE][SQUARE_E1][PIECE_KING] ^ _square_piece_hashes[COLOR_WHITE][SQUARE_G1][PIECE_KING];
		_castling_hash_wk ^= _square_piece_hashes[COLOR_WHITE][SQUARE_H1][PIECE_ROOK] ^ _square_piece_hashes[COLOR_WHITE][SQUARE_F1][PIECE_ROOK];
		_castling_hash_wq ^= _square_piece_hashes[COLOR_WHITE][SQUARE_E1][PIECE_KING] ^ _square_piece_hashes[COLOR_WHITE][SQUARE_C1][PIECE_KING];
		_castling_hash_wq ^= _square_piece_hashes[COLOR_WHITE][SQUARE_A1][PIECE_ROOK] ^ _square_piece_hashes[COLOR_WHITE][SQUARE_D1][PIECE_ROOK];
		_castling_hash_bk ^= _square_piece_hashes[COLOR_BLACK][SQUARE_E8][PIECE_KING] ^ _square_piece_hashes[COLOR_BLACK][SQUARE_G8][PIECE_KING];
		_castling_hash_bk ^= _square_piece_hashes[COLOR_BLACK][SQUARE_H8][PIECE_ROOK] ^ _square_piece_hashes[COLOR_BLACK][SQUARE_F8][PIECE_ROOK];
		_castling_hash_bq ^= _square_piece_hashes[COLOR_BLACK][SQUARE_E8][PIECE_KING] ^ _square_piece_hashes[COLOR_BLACK][SQUARE_C8][PIECE_KING];
		_castling_hash_bq ^= _square_piece_hashes[COLOR_BLACK][SQUARE_A8][PIECE_ROOK] ^ _square_piece_hashes[COLOR_BLACK][SQUARE_D8][PIECE_ROOK];
	}

private:
	Hash _square_piece_hashes[COLOR_COUNT][SQUARE_COUNT][PIECE_COUNT] = {};
	Hash _castling_hash_wk = 0;
	Hash _castling_hash_wq = 0;
	Hash _castling_hash_bk = 0;
	Hash _castling_hash_bq = 0;
};

inline const Hasher hasher;
