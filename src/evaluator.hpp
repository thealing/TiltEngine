#pragma once

#include "position.hpp"
#include "values.hpp"

#include <numeric>

// for testing new evaluation terms
#define ALWAYS_REEVALUATE true

struct Evaluation
{
	Value value;
	Score weight;
};

class Evaluator
{
public:
	inline void evaluate_position(Evaluation* evaluation, const Position& position) const
	{
		evaluation->value = get_position_value(position);
		evaluation->weight = get_position_weight(position);
	}

	template<Color color>
	inline void update_evaluation(Evaluation* dst_evaluation, const Evaluation* src_evaluation, const Move& move) const
	{
		dst_evaluation->value = src_evaluation->value + get_move_value<color>(move);
		dst_evaluation->weight = src_evaluation->weight + get_move_weight<color>(move);
	}

	inline Score get_evaluation_score(const Evaluation& evaluation) const
	{
		return interpolate_phases(evaluation.value, evaluation.weight);
	}

private:
	inline Value get_position_value(const Position& position) const
	{
		Value value = {};
		for (Color color = 0; color < COLOR_COUNT; color++)
		{
			for (Piece piece = 0; piece < PIECE_COUNT; piece++)
			{
				Bitboard mask = position.colors[color] & position.pieces[piece];
				while (mask != 0)
				{
					Square square = pop_square(mask);
					value += _square_values[color][square][piece];
				}
			}
		}
		value += get_mobility_value(position);
		return value;
	}

	inline Value get_mobility_value(const Position& position) const
	{
		Value value = 0;
		Bitboard occupied_mask = position.colors[COLOR_WHITE] | position.colors[COLOR_BLACK];
		Bitboard empty_mask = ~occupied_mask;
		Bitboard pawn_move_masks[COLOR_COUNT];
		Bitboard pawn_capture_masks[COLOR_COUNT];
		get_pawn_masks<COLOR_WHITE>(position, pawn_move_masks[COLOR_WHITE], pawn_capture_masks[COLOR_WHITE]);
		get_pawn_masks<COLOR_BLACK>(position, pawn_move_masks[COLOR_BLACK], pawn_capture_masks[COLOR_BLACK]);
		for (Color color = 0; color < COLOR_COUNT; color++)
		{
			Color opponent_color = flip_color(color);
			Bitboard color_mask =  position.colors[color];
			Bitboard opponent_mask =  position.colors[opponent_color];
			value += _move_values[PIECE_PAWN] * count_squares(pawn_move_masks[color]);
			value += _attack_values[PIECE_PAWN] * count_squares(opponent_mask & pawn_capture_masks[color]);
			value += _defense_values[PIECE_PAWN] * count_squares(color_mask & pawn_capture_masks[color]);
			for (Piece piece = PIECE_KNIGHT; piece <= PIECE_KING; piece++)
			{
				Bitboard src_mask = color_mask & position.pieces[piece];
				while (src_mask != 0)
				{
					Square src_square = pop_square(src_mask);
					Bitboard dst_mask = 0;
					switch (piece)
					{
						case PIECE_KNIGHT:
							dst_mask = bitmasks.get_knight_mask(src_square);
							break;
						case PIECE_BISHOP:
							dst_mask = magics.get_bishop_mask(src_square, occupied_mask);
							break;
						case PIECE_ROOK:
							dst_mask = magics.get_rook_mask(src_square, occupied_mask);
							break;
						case PIECE_QUEEN:
							dst_mask = magics.get_queen_mask(src_square, occupied_mask);
							break;
						case PIECE_KING:
							dst_mask = bitmasks.get_king_mask(src_square);
							break;
					}
					dst_mask &= ~pawn_capture_masks[opponent_color];
					value += _move_values[piece] * count_squares(dst_mask & empty_mask);
					value += _attack_values[piece] * count_squares(dst_mask & opponent_mask);
					value += _defense_values[piece] * count_squares(dst_mask & color_mask);
				}
			}
			value *= -1;
		}
		return value;
	}

	template<Color color>
	inline void get_pawn_masks(const Position& position, Bitboard& pawn_move_mask, Bitboard& pawn_capture_mask) const
	{
		const Bitboard color_mask = position.colors[color];
		const Bitboard opponent_mask = position.colors[flip_color(color)];
		const Bitboard occupied_mask = color_mask | opponent_mask;
		const Bitboard empty_mask = ~occupied_mask;
		const Bitboard pawn_mask = color_mask & position.pieces[PIECE_PAWN];
		pawn_move_mask = empty_mask & shift_forward<color>(pawn_mask);
		const Bitboard pawn_left_capture_mask = shift_forward_left<color>(pawn_mask) & ~get_file_mask(FILE_H);
		const Bitboard pawn_right_capture_mask = shift_forward_right<color>(pawn_mask) & ~get_file_mask(FILE_A);
		pawn_capture_mask = pawn_left_capture_mask | pawn_right_capture_mask;
	}

	inline Score get_position_weight(const Position& position) const
	{
		Score weight = 0;
		for (Piece piece = 0; piece < PIECE_COUNT; piece++)
		{
			Bitboard mask = position.pieces[piece];
			weight += _weights[piece] * count_squares(mask);
		}
		return weight;
	}

	inline Score interpolate_phases(Value value, Score weight) const
	{
		return (get_opening_score(value) * weight + get_endgame_score(value) * (_total_weight - weight)) / _total_weight;
	}

	template<Color color>
	inline Value get_move_value(const Move& move) const
	{
		constexpr Color opponent_color = flip_color(color);
		Value value = {};
		switch (move.type)
		{
			case MOVE_TYPE_DOUBLE:
				value -= _square_values[color][move.src_square][PIECE_PAWN];
				value += _square_values[color][move.dst_square][PIECE_PAWN];
				break;
			case MOVE_TYPE_EN_PASSANT:
				value -= _square_values[color][move.src_square][PIECE_PAWN];
				value += _square_values[color][move.dst_square][PIECE_PAWN];
				value -= _square_values[opponent_color][move_backward<color>(move.dst_square)][PIECE_PAWN];
				break;
			case MOVE_TYPE_PROMOTION_Q:
				value -= _square_values[color][move.src_square][PIECE_PAWN];
				value += _square_values[color][move.dst_square][PIECE_QUEEN];
				break;
			case MOVE_TYPE_PROMOTION_R:
				value -= _square_values[color][move.src_square][PIECE_PAWN];
				value += _square_values[color][move.dst_square][PIECE_ROOK];
				break;
			case MOVE_TYPE_PROMOTION_B:
				value -= _square_values[color][move.src_square][PIECE_PAWN];
				value += _square_values[color][move.dst_square][PIECE_BISHOP];
				break;
			case MOVE_TYPE_PROMOTION_N:
				value -= _square_values[color][move.src_square][PIECE_PAWN];
				value += _square_values[color][move.dst_square][PIECE_KNIGHT];
				break;
			case MOVE_TYPE_CASTLING_WK:
				value += _castling_value_wk;
				break;
			case MOVE_TYPE_CASTLING_WQ:
				value += _castling_value_wq;
				break;
			case MOVE_TYPE_CASTLING_BK:
				value += _castling_value_bk;
				break;
			case MOVE_TYPE_CASTLING_BQ:
				value += _castling_value_bq;
				break;
			default:
				value -= _square_values[color][move.src_square][move.type];
				value += _square_values[color][move.dst_square][move.type];
				break;
		}
		if (move.captured_piece != PIECE_NONE)
		{
			value -= _square_values[opponent_color][move.dst_square][move.captured_piece];
		}
		return value;
	}

	template<Color color>
	inline Score get_move_weight(const Move& move) const
	{
		Score weight = 0;
		switch (move.type)
		{
			case MOVE_TYPE_EN_PASSANT:
				weight -= _weights[PIECE_PAWN];
				break;
			case MOVE_TYPE_PROMOTION_Q:
				weight -= _weights[PIECE_PAWN];
				weight += _weights[PIECE_QUEEN];
				break;
			case MOVE_TYPE_PROMOTION_R:
				weight -= _weights[PIECE_PAWN];
				weight += _weights[PIECE_ROOK];
				break;
			case MOVE_TYPE_PROMOTION_B:
				weight -= _weights[PIECE_PAWN];
				weight += _weights[PIECE_BISHOP];
				break;
			case MOVE_TYPE_PROMOTION_N:
				weight -= _weights[PIECE_PAWN];
				weight += _weights[PIECE_KNIGHT];
				break;
		}
		if (move.captured_piece != PIECE_NONE)
		{
			weight -= _weights[move.captured_piece];
		}
		return weight;
	}

public:
	inline constexpr Evaluator()
	{
		constexpr Score opening_move_values[PIECE_COUNT] = { 13, 17, 17, 10, 5, -9 };
		constexpr Score endgame_move_values[PIECE_COUNT] = { 21, 12, 13, 14, 29, -8 };
		constexpr Score opening_attack_values[PIECE_COUNT] = { 65, 16, 29, 26, 2, -84 };
		constexpr Score endgame_attack_values[PIECE_COUNT] = { 17, 27, 44, 48, 74, 24 };
		constexpr Score opening_defense_values[PIECE_COUNT] = { 13, 14, 19, 11, 5, 3 };
		constexpr Score endgame_defense_values[PIECE_COUNT] = { 8, 13, 17, 32, 53, -5 };
		constexpr Score opening_pawn_table[SQUARE_COUNT] = {
			100, 100, 100, 100, 100, 100, 100, 100,
			178, 194, 167, 192, 166, 170, 129, 109,
			101, 102, 122, 132, 169, 178, 137, 84,
			72, 86, 84, 95, 92, 102, 92, 62,
			60, 69, 82, 93, 107, 95, 90, 60,
			59, 76, 73, 78, 96, 94, 126, 72,
			71, 92, 80, 85, 90, 135, 145, 81,
			100, 100, 100, 100, 100, 100, 100, 100,
		};
		constexpr Score endgame_pawn_table[SQUARE_COUNT] = {
			100, 100, 100, 100, 100, 100, 100, 100,
			380, 370, 338, 315, 327, 316, 346, 370,
			248, 255, 237, 216, 208, 202, 238, 234,
			162, 149, 136, 130, 122, 129, 142, 141,
			133, 124, 110, 106, 107, 103, 116, 112,
			120, 117, 105, 114, 109, 105, 107, 103,
			131, 122, 118, 118, 124, 113, 112, 103,
			100, 100, 100, 100, 100, 100, 100, 100,
		};
		constexpr Score opening_knight_table[SQUARE_COUNT] = {
			240, 286, 293, 298, 314, 277, 295, 278,
			259, 278, 393, 325, 323, 352, 314, 300,
			280, 354, 346, 368, 398, 399, 387, 333,
			324, 351, 319, 397, 370, 397, 369, 368,
			337, 323, 335, 344, 365, 354, 355, 356,
			322, 314, 327, 323, 345, 339, 360, 332,
			317, 286, 312, 347, 341, 352, 322, 345,
			275, 342, 275, 303, 336, 307, 348, 299,
		};
		constexpr Score endgame_knight_table[SQUARE_COUNT] = {
			275, 296, 329, 304, 324, 294, 283, 261,
			311, 335, 319, 342, 334, 319, 323, 292,
			309, 321, 329, 331, 325, 327, 317, 308,
			338, 328, 340, 346, 339, 331, 326, 336,
			329, 326, 331, 344, 339, 335, 340, 334,
			327, 327, 309, 328, 324, 311, 311, 324,
			310, 317, 318, 325, 328, 313, 323, 302,
			300, 314, 321, 334, 330, 326, 309, 287,
		};
		constexpr Score opening_bishop_table[SQUARE_COUNT] = {
			318, 310, 277, 289, 297, 289, 304, 318,
			297, 331, 284, 289, 321, 333, 329, 260,
			344, 335, 351, 332, 326, 349, 353, 376,
			333, 366, 323, 365, 340, 359, 366, 355,
			339, 350, 355, 341, 375, 330, 350, 362,
			371, 368, 355, 362, 356, 386, 360, 385,
			346, 382, 370, 354, 365, 365, 405, 348,
			320, 350, 377, 343, 357, 357, 304, 335,
		};
		constexpr Score endgame_bishop_table[SQUARE_COUNT] = {
			327, 316, 313, 321, 324, 318, 319, 316,
			334, 321, 327, 308, 322, 322, 319, 321,
			341, 318, 310, 309, 311, 315, 322, 341,
			344, 320, 318, 302, 302, 318, 314, 344,
			339, 319, 317, 311, 301, 318, 321, 340,
			337, 327, 324, 329, 329, 319, 326, 338,
			335, 323, 330, 336, 339, 323, 325, 323,
			327, 348, 347, 345, 342, 346, 336, 331,
		};
		constexpr Score opening_rook_table[SQUARE_COUNT] = {
			514, 522, 503, 536, 532, 506, 509, 510,
			508, 523, 541, 548, 550, 546, 514, 519,
			478, 508, 515, 521, 511, 524, 541, 506,
			467, 481, 495, 522, 517, 531, 497, 481,
			458, 473, 497, 497, 520, 499, 523, 482,
			458, 480, 498, 499, 515, 521, 507, 472,
			460, 501, 495, 513, 531, 531, 518, 435,
			496, 507, 527, 538, 544, 516, 487, 502,
		};
		constexpr Score endgame_rook_table[SQUARE_COUNT] = {
			567, 565, 569, 568, 570, 568, 566, 560,
			560, 561, 557, 555, 547, 557, 564, 561,
			567, 567, 562, 568, 558, 550, 555, 557,
			567, 558, 569, 556, 560, 554, 547, 568,
			560, 555, 556, 549, 545, 536, 539, 546,
			548, 548, 535, 537, 528, 524, 538, 536,
			546, 542, 544, 545, 533, 531, 534, 549,
			542, 551, 550, 546, 543, 538, 547, 528,
		};
		constexpr Score opening_queen_table[SQUARE_COUNT] = {
			907, 912, 922, 916, 951, 935, 937, 956,
			882, 850, 892, 906, 897, 939, 940, 964,
			901, 885, 911, 907, 932, 945, 956, 986,
			878, 879, 874, 883, 899, 925, 921, 928,
			905, 872, 898, 878, 903, 899, 920, 919,
			892, 911, 884, 898, 893, 907, 920, 924,
			884, 891, 917, 907, 918, 911, 897, 918,
			907, 907, 912, 924, 900, 880, 903, 882,
		};
		constexpr Score endgame_queen_table[SQUARE_COUNT] = {
			909, 933, 936, 933, 948, 942, 934, 952,
			907, 904, 900, 919, 928, 927, 918, 947,
			914, 888, 851, 906, 910, 910, 918, 957,
			946, 901, 869, 860, 885, 897, 947, 970,
			910, 913, 875, 873, 867, 890, 928, 958,
			926, 868, 884, 875, 869, 898, 917, 940,
			929, 901, 884, 898, 897, 898, 895, 912,
			928, 913, 916, 918, 942, 919, 912, 897,
		};
		constexpr Score opening_king_table[SQUARE_COUNT] = {
			-6, 2, 3, 0, -7, -2, 2, 0,
			3, 14, 11, 17, 8, 13, 8, -8,
			4, 22, 27, 7, 14, 31, 33, -2,
			-4, 13, 19, 4, 4, 11, 21, -22,
			-21, 17, 0, -29, -33, -16, 1, -47,
			-6, 24, -8, -35, -46, -30, 20, -27,
			0, 53, 0, -66, -48, -5, 53, 40,
			-51, 57, 14, -94, 9, -43, 52, 21,
		};
		constexpr Score endgame_king_table[SQUARE_COUNT] = {
			-38, -26, -19, -19, -15, 6, 0, -15,
			-12, 26, 28, 29, 27, 52, 33, 2,
			-1, 28, 35, 26, 30, 60, 60, -3,
			-26, 26, 33, 37, 29, 37, 24, -23,
			-46, -2, 27, 30, 33, 20, 2, -40,
			-41, -1, 15, 26, 28, 24, 10, -32,
			-53, -9, 14, 19, 23, 15, 0, -44,
			-104, -62, -38, -27, -37, -27, -50, -103,
		};
		const Score* opening_piece_tables[] = {
			opening_pawn_table,
			opening_knight_table,
			opening_bishop_table,
			opening_rook_table,
			opening_queen_table,
			opening_king_table
		};
		const Score* endgame_piece_tables[] = {
			endgame_pawn_table,
			endgame_knight_table,
			endgame_bishop_table,
			endgame_rook_table,
			endgame_queen_table,
			endgame_king_table
		};
		for (Piece piece = 0; piece < PIECE_COUNT; piece++)
		{
			_move_values[piece] = make_value(opening_move_values[piece], endgame_move_values[piece]);
			_attack_values[piece] = make_value(opening_attack_values[piece], endgame_attack_values[piece]);
			_defense_values[piece] = make_value(opening_defense_values[piece], endgame_defense_values[piece]);
		}
		for (Square square = 0; square < SQUARE_COUNT; square++)
		{
			for (Piece piece = 0; piece < PIECE_COUNT; piece++)
			{
				Value value = make_value(opening_piece_tables[piece][square], endgame_piece_tables[piece][square]);
				_square_values[COLOR_WHITE][square][piece] = value;
				_square_values[COLOR_BLACK][mirror_rank(square)][piece] = Value{} - value;
			}
		}
		_castling_value_wk += _square_values[COLOR_WHITE][SQUARE_G1][PIECE_KING] - _square_values[COLOR_WHITE][SQUARE_E1][PIECE_KING];
		_castling_value_wk += _square_values[COLOR_WHITE][SQUARE_F1][PIECE_ROOK] - _square_values[COLOR_WHITE][SQUARE_H1][PIECE_ROOK];
		_castling_value_wq += _square_values[COLOR_WHITE][SQUARE_C1][PIECE_KING] - _square_values[COLOR_WHITE][SQUARE_E1][PIECE_KING];
		_castling_value_wq += _square_values[COLOR_WHITE][SQUARE_D1][PIECE_ROOK] - _square_values[COLOR_WHITE][SQUARE_A1][PIECE_ROOK];
		_castling_value_bk += _square_values[COLOR_BLACK][SQUARE_G8][PIECE_KING] - _square_values[COLOR_BLACK][SQUARE_E8][PIECE_KING];
		_castling_value_bk += _square_values[COLOR_BLACK][SQUARE_F8][PIECE_ROOK] - _square_values[COLOR_BLACK][SQUARE_H8][PIECE_ROOK];
		_castling_value_bq += _square_values[COLOR_BLACK][SQUARE_C8][PIECE_KING] - _square_values[COLOR_BLACK][SQUARE_E8][PIECE_KING];
		_castling_value_bq += _square_values[COLOR_BLACK][SQUARE_D8][PIECE_ROOK] - _square_values[COLOR_BLACK][SQUARE_A8][PIECE_ROOK];
	}

private:
	constexpr static Score _weights[PIECE_COUNT] = { -55, 139, 212, 368, 1014, -138 };
	constexpr static Score _total_weight = 3748;

	Value _move_values[PIECE_COUNT] = {};
	Value _attack_values[PIECE_COUNT] = {};
	Value _defense_values[PIECE_COUNT] = {};
	Value _passed_pawn_values[SQUARE_COUNT] = {};
	Value _square_values[COLOR_COUNT][SQUARE_COUNT][PIECE_COUNT] = {};
	Value _castling_value_wk = {};
	Value _castling_value_wq = {};
	Value _castling_value_bk = {};
	Value _castling_value_bq = {};
};

inline const Evaluator evaluator;
