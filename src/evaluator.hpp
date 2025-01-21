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
		const Bitboard occupied_mask = position.colors[COLOR_WHITE] | position.colors[COLOR_BLACK];
		const Bitboard empty_mask = ~occupied_mask;
		for (Color color = 0; color < COLOR_COUNT; color++)
		{
			Bitboard color_mask =  position.colors[color];
			Bitboard opponent_mask =  position.colors[flip_color(color)];
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
					uint64_t move_mask = dst_mask & empty_mask;
					uint64_t attack_mask = dst_mask & opponent_mask;
					uint64_t defense_mask = dst_mask & color_mask;
					value += _move_values[piece] * count_squares(move_mask);
					value += _attack_values[piece] * count_squares(attack_mask);
					value += _defense_values[piece] * count_squares(defense_mask);
				}
			}
			value *= -1;
		}
		value += get_pawn_value<COLOR_WHITE>(position);
		value -= get_pawn_value<COLOR_BLACK>(position);
		return value;
	}

	template<Color color>
	inline Value get_pawn_value(const Position& position) const
	{
		const Bitboard color_mask = position.colors[color];
		const Bitboard opponent_mask = position.colors[flip_color(color)];
		const Bitboard occupied_mask = color_mask | opponent_mask;
		const Bitboard empty_mask = ~occupied_mask;
		const Bitboard pawn_mask = color_mask & position.pieces[PIECE_PAWN];
		const Bitboard pawn_move_mask = empty_mask & shift_forward<color>(pawn_mask);
		const Bitboard pawn_left_capture_mask = shift_forward_left<color>(pawn_mask) & ~get_file_mask(FILE_H);
		const Bitboard pawn_right_capture_mask = shift_forward_right<color>(pawn_mask) & ~get_file_mask(FILE_A);
		const Bitboard pawn_capture_mask = pawn_left_capture_mask | pawn_right_capture_mask;
		const Bitboard pawn_attack_mask = opponent_mask & pawn_capture_mask;
		const Bitboard pawn_defense_mask = color_mask & pawn_capture_mask;
		Value value = 0;
		value += _move_values[PIECE_PAWN] * count_squares(pawn_move_mask);
		value += _attack_values[PIECE_PAWN] * count_squares(pawn_attack_mask);
		value += _defense_values[PIECE_PAWN] * count_squares(pawn_defense_mask);
		return value;
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
		constexpr Score opening_move_values[PIECE_COUNT] = { 12, 24, 20, 9, 5, -13 };
		constexpr Score endgame_move_values[PIECE_COUNT] = { 31, 13, 13, 15, 27, 16 };
		constexpr Score opening_attack_values[PIECE_COUNT] = { 61, 14, 27, 16, -3, -79 };
		constexpr Score endgame_attack_values[PIECE_COUNT] = { 22, 24, 39, 50, 64, 47 };
		constexpr Score opening_defense_values[PIECE_COUNT] = { 14, 23, 37, 12, 5, 1 };
		constexpr Score endgame_defense_values[PIECE_COUNT] = { 13, 17, 20, 43, 50, 19 };
		constexpr Score opening_pawn_table[SQUARE_COUNT] = {
			100, 100, 100, 100, 100, 100, 100, 100,
			169, 175, 155, 162, 146, 146, 135, 132,
			99, 111, 131, 132, 151, 150, 130, 88,
			71, 113, 98, 120, 124, 116, 124, 64,
			58, 88, 104, 119, 132, 117, 111, 64,
			65, 86, 97, 98, 115, 120, 144, 85,
			66, 102, 86, 98, 108, 143, 158, 81,
			100, 100, 100, 100, 100, 100, 100, 100,
		};
		constexpr Score endgame_pawn_table[SQUARE_COUNT] = {
			100, 100, 100, 100, 100, 100, 100, 100,
			350, 333, 292, 272, 254, 254, 273, 310,
			239, 249, 227, 205, 193, 190, 225, 223,
			155, 141, 128, 120, 107, 117, 128, 131,
			127, 118, 102, 97, 96, 91, 105, 103,
			112, 112, 97, 106, 99, 96, 94, 92,
			124, 113, 111, 104, 109, 102, 100, 92,
			100, 100, 100, 100, 100, 100, 100, 100,
		};
		constexpr Score opening_knight_table[SQUARE_COUNT] = {
			278, 294, 298, 296, 305, 286, 297, 291,
			282, 291, 334, 302, 298, 310, 304, 297,
			289, 314, 298, 309, 325, 322, 329, 308,
			311, 317, 263, 318, 289, 328, 329, 335,
			312, 296, 271, 259, 292, 275, 309, 322,
			297, 279, 265, 272, 287, 281, 319, 309,
			307, 291, 275, 304, 306, 306, 300, 322,
			292, 335, 282, 293, 316, 289, 339, 298,
		};
		constexpr Score endgame_knight_table[SQUARE_COUNT] = {
			284, 293, 310, 295, 307, 288, 290, 284,
			299, 315, 297, 314, 302, 294, 303, 290,
			295, 299, 301, 302, 297, 294, 298, 294,
			321, 306, 308, 315, 305, 300, 302, 317,
			309, 298, 304, 311, 304, 300, 312, 310,
			307, 307, 277, 297, 289, 283, 288, 303,
			296, 297, 292, 296, 301, 287, 301, 295,
			296, 285, 295, 309, 305, 301, 284, 292,
		};
		constexpr Score opening_bishop_table[SQUARE_COUNT] = {
			306, 301, 291, 296, 300, 296, 301, 305,
			297, 307, 285, 290, 305, 311, 309, 266,
			320, 309, 308, 297, 300, 310, 318, 352,
			315, 324, 290, 303, 286, 305, 328, 326,
			322, 309, 289, 281, 301, 278, 303, 332,
			354, 326, 304, 288, 295, 332, 322, 358,
			322, 345, 321, 311, 311, 316, 367, 319,
			308, 324, 354, 320, 329, 326, 300, 316,
		};
		constexpr Score endgame_bishop_table[SQUARE_COUNT] = {
			308, 303, 301, 306, 311, 305, 306, 303,
			319, 305, 308, 288, 304, 307, 305, 302,
			327, 304, 294, 291, 291, 296, 307, 327,
			333, 304, 298, 284, 282, 295, 294, 328,
			322, 305, 297, 286, 271, 294, 299, 323,
			319, 309, 302, 298, 295, 291, 312, 318,
			314, 302, 300, 308, 313, 303, 296, 304,
			306, 319, 318, 323, 321, 323, 311, 310,
		};
		constexpr Score opening_rook_table[SQUARE_COUNT] = {
			516, 516, 509, 521, 517, 506, 507, 510,
			517, 521, 530, 527, 524, 522, 508, 514,
			495, 504, 505, 505, 500, 503, 514, 502,
			486, 484, 493, 506, 499, 510, 490, 492,
			470, 482, 496, 488, 504, 489, 501, 485,
			466, 482, 491, 493, 501, 503, 491, 474,
			468, 500, 495, 509, 519, 518, 501, 444,
			502, 509, 524, 535, 544, 510, 488, 508,
		};
		constexpr Score endgame_rook_table[SQUARE_COUNT] = {
			540, 533, 537, 536, 534, 530, 526, 526,
			532, 529, 529, 528, 516, 521, 525, 530,
			521, 515, 510, 510, 506, 499, 507, 515,
			518, 502, 511, 502, 499, 501, 490, 518,
			515, 503, 504, 494, 491, 480, 488, 500,
			509, 502, 489, 492, 480, 479, 491, 497,
			512, 501, 502, 504, 491, 491, 494, 512,
			510, 514, 512, 505, 498, 504, 511, 494,
		};
		constexpr Score opening_queen_table[SQUARE_COUNT] = {
			917, 916, 918, 915, 931, 920, 920, 941,
			898, 868, 902, 907, 902, 925, 927, 945,
			903, 885, 892, 894, 912, 918, 931, 967,
			892, 871, 867, 865, 880, 904, 912, 927,
			902, 869, 879, 863, 879, 881, 904, 920,
			899, 907, 876, 888, 878, 897, 913, 921,
			896, 894, 915, 909, 918, 905, 891, 912,
			915, 907, 910, 920, 908, 885, 901, 894,
		};
		constexpr Score endgame_queen_table[SQUARE_COUNT] = {
			913, 926, 927, 925, 937, 928, 921, 938,
			910, 898, 905, 912, 914, 923, 917, 936,
			910, 888, 872, 898, 904, 905, 915, 946,
			922, 892, 870, 863, 877, 894, 922, 943,
			911, 896, 876, 870, 867, 883, 914, 935,
			914, 879, 878, 864, 865, 889, 910, 921,
			912, 896, 879, 884, 883, 893, 890, 907,
			917, 902, 904, 920, 914, 901, 904, 899,
		};
		constexpr Score opening_king_table[SQUARE_COUNT] = {
			0, 1, 2, 1, 0, 2, 3, 1,
			3, 3, 3, 4, 2, 5, 2, 1,
			5, 6, 8, 2, 4, 12, 12, 5,
			1, 3, 5, 3, 1, 3, 2, -4,
			-6, 0, 0, -10, -10, -8, -10, -16,
			0, 0, -5, -16, -24, -15, 5, -14,
			-1, 13, -1, -51, -37, -2, 56, 36,
			-24, 48, 14, -67, 12, -50, 54, 21,
		};
		constexpr Score endgame_king_table[SQUARE_COUNT] = {
			-3, 0, 2, 4, 8, 16, 14, 10,
			6, 5, 5, 4, 3, 22, 9, 18,
			25, 2, 10, 0, 4, 33, 33, 32,
			9, -1, 7, 12, 4, 14, 0, 16,
			-6, -28, 1, 4, 10, 0, -19, 0,
			-4, -27, -9, 0, 2, 2, -12, 5,
			-17, -34, -15, -7, -3, -14, -34, -10,
			-30, -33, -10, 6, -8, 10, -23, -30,
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
	constexpr static Score _weights[PIECE_COUNT] = { -56, 235, 267, 440, 961, -62 };
	constexpr static Score _total_weight = 4670;

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
