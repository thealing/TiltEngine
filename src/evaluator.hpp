#pragma once

#include "position.hpp"
#include "values.hpp"
#include "pawn_structure_table.hpp"

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
		return get_position_value<COLOR_WHITE>(position) - get_position_value<COLOR_BLACK>(position);
	}

	template<Color color>
	inline Value get_position_value(const Position& position) const
	{
		Value value = 0;
		value += get_piece_square_value<color>(position);
		value += get_mobility_value<color>(position);
		return value;
	}

	template<Color color>
	inline Value get_piece_square_value(const Position& position) const
	{
		Value value = 0;
		for (Piece piece = 0; piece < PIECE_COUNT; piece++)
		{
			Bitboard mask = position.get_mask(piece, color);
			while (mask != 0)
			{
				Square square = pop_square(mask);
				value += _square_values[color][square][piece];
			}
		}
		return value;
	}

	template<Color color>
	inline Value get_mobility_value(const Position& position) const
	{
		constexpr Color enemy = flip_color(color);
		Value value = 0;
		PawnStructureEntry entry(position);
		Bitboard color_mask =  position.colors[color];
		Bitboard enemy_mask =  position.colors[enemy];
		Bitboard occupied_mask = color_mask | enemy_mask;
		Bitboard empty_mask = ~occupied_mask;
		value += _move_values[PIECE_PAWN] * count_squares(empty_mask & entry.move_masks[color]);
		value += _attack_values[PIECE_PAWN] * count_squares(enemy_mask & entry.attack_masks[color]);
		value += _defense_values[PIECE_PAWN] * count_squares(color_mask & entry.attack_masks[color]);
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
						dst_mask = sliders.get_bishop_mask(src_square, occupied_mask);
						break;
					case PIECE_ROOK:
						dst_mask = sliders.get_rook_mask(src_square, occupied_mask);
						break;
					case PIECE_QUEEN:
						dst_mask = sliders.get_queen_mask(src_square, occupied_mask);
						break;
					case PIECE_KING:
						dst_mask = bitmasks.get_king_mask(src_square);
						break;
				}
				dst_mask &= ~entry.attack_masks[enemy];
				value += _move_values[piece] * count_squares(dst_mask & empty_mask);
				value += _attack_values[piece] * count_squares(dst_mask & enemy_mask);
				value += _defense_values[piece] * count_squares(dst_mask & color_mask);
			}
		}
		// pawns
		Bitboard mask;
		mask = entry.passed_masks[color];
		while (mask != 0)
		{
			Square square = pop_square(mask);
			value += _passed_pawn_values[color][square];
		}
		mask = entry.doubled_masks[color];
		while (mask != 0)
		{
			Square square = pop_square(mask);
			value += _doubled_pawn_values[color][square];
		}
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
		constexpr Color enemy = flip_color(color);
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
				value -= _square_values[enemy][move_backward<color>(move.dst_square)][PIECE_PAWN];
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
			value -= _square_values[enemy][move.dst_square][move.captured_piece];
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
		constexpr Score opening_move_values[PIECE_COUNT] = { 14, 20, 17, 9, 6, 2 };
		constexpr Score endgame_move_values[PIECE_COUNT] = { 17, 16, 14, 15, 28, -9 };
		constexpr Score opening_attack_values[PIECE_COUNT] = { 65, 19, 30, 25, 5, -57 };
		constexpr Score endgame_attack_values[PIECE_COUNT] = { 16, 31, 44, 54, 65, 26 };
		constexpr Score opening_defense_values[PIECE_COUNT] = { 13, 17, 21, 8, 7, 15 };
		constexpr Score endgame_defense_values[PIECE_COUNT] = { 10, 16, 17, 34, 51, -5 };
		constexpr Score opening_pawn_table[SQUARE_COUNT] = {
			100, 100, 100, 100, 100, 100, 100, 100,
			136, 142, 130, 142, 130, 137, 109, 96,
			91, 89, 94, 102, 131, 153, 128, 94,
			73, 79, 81, 95, 90, 95, 85, 64,
			65, 66, 82, 92, 105, 92, 83, 61,
			64, 74, 73, 78, 95, 91, 119, 72,
			77, 89, 82, 86, 89, 131, 136, 82,
			100, 100, 100, 100, 100, 100, 100, 100,
		};
		constexpr Score endgame_pawn_table[SQUARE_COUNT] = {
			100, 100, 100, 100, 100, 100, 100, 100,
			243, 238, 221, 209, 217, 214, 229, 241,
			141, 144, 153, 142, 150, 141, 159, 140,
			116, 115, 109, 101, 110, 115, 117, 111,
			105, 107, 101, 97, 102, 100, 103, 93,
			94, 97, 96, 101, 104, 101, 93, 87,
			104, 99, 108, 105, 120, 108, 99, 86,
			100, 100, 100, 100, 100, 100, 100, 100,
		};
		constexpr Score opening_knight_table[SQUARE_COUNT] = {
			256, 289, 293, 296, 308, 283, 296, 285,
			266, 276, 367, 310, 310, 327, 310, 299,
			275, 320, 303, 320, 346, 356, 352, 320,
			311, 321, 284, 365, 337, 351, 338, 345,
			313, 303, 299, 313, 331, 321, 333, 336,
			300, 282, 294, 281, 303, 304, 328, 308,
			300, 283, 280, 317, 309, 316, 301, 326,
			282, 317, 268, 285, 318, 282, 323, 296,
		};
		constexpr Score endgame_knight_table[SQUARE_COUNT] = {
			276, 292, 316, 296, 315, 290, 287, 271,
			303, 322, 307, 327, 322, 308, 316, 294,
			297, 306, 306, 311, 308, 308, 306, 301,
			323, 312, 317, 325, 319, 312, 311, 326,
			321, 308, 310, 325, 320, 314, 320, 326,
			318, 309, 288, 308, 306, 289, 295, 314,
			308, 307, 302, 311, 312, 300, 311, 299,
			301, 313, 305, 319, 317, 312, 308, 289,
		};
		constexpr Score opening_bishop_table[SQUARE_COUNT] = {
			316, 306, 281, 291, 298, 289, 303, 314,
			295, 314, 277, 290, 316, 317, 315, 263,
			328, 313, 328, 309, 304, 325, 333, 355,
			315, 352, 307, 344, 318, 339, 350, 340,
			336, 325, 340, 319, 358, 314, 334, 356,
			358, 355, 336, 347, 339, 368, 343, 373,
			342, 366, 355, 338, 349, 340, 388, 335,
			317, 347, 363, 337, 341, 341, 300, 333,
		};
		constexpr Score endgame_bishop_table[SQUARE_COUNT] = {
			323, 315, 312, 317, 321, 316, 319, 316,
			333, 320, 322, 308, 321, 323, 319, 319,
			339, 315, 307, 306, 308, 312, 320, 341,
			341, 316, 313, 300, 298, 315, 311, 342,
			339, 318, 314, 309, 299, 314, 317, 339,
			339, 328, 323, 328, 326, 316, 324, 342,
			339, 325, 329, 334, 337, 324, 325, 322,
			326, 349, 350, 344, 340, 347, 333, 331,
		};
		constexpr Score opening_rook_table[SQUARE_COUNT] = {
			513, 516, 498, 527, 523, 505, 508, 508,
			507, 514, 524, 534, 530, 529, 506, 513,
			483, 505, 513, 520, 512, 520, 530, 508,
			472, 481, 496, 517, 517, 524, 498, 487,
			464, 477, 496, 496, 517, 498, 517, 482,
			460, 478, 497, 499, 515, 518, 505, 471,
			462, 500, 495, 513, 530, 527, 513, 436,
			495, 506, 525, 536, 542, 513, 485, 497,
		};
		constexpr Score endgame_rook_table[SQUARE_COUNT] = {
			546, 546, 549, 548, 552, 551, 550, 541,
			540, 542, 539, 536, 531, 542, 550, 542,
			548, 549, 544, 549, 539, 533, 538, 536,
			547, 539, 549, 537, 542, 536, 527, 547,
			543, 536, 537, 528, 525, 516, 521, 529,
			531, 527, 515, 516, 508, 503, 515, 517,
			528, 523, 522, 524, 513, 508, 513, 525,
			523, 530, 530, 526, 524, 519, 526, 510,
		};
		constexpr Score opening_queen_table[SQUARE_COUNT] = {
			912, 911, 923, 915, 944, 930, 931, 955,
			885, 852, 890, 906, 897, 931, 930, 963,
			903, 886, 906, 904, 929, 944, 950, 989,
			882, 878, 869, 878, 891, 921, 923, 927,
			903, 868, 891, 872, 893, 891, 914, 919,
			892, 907, 877, 892, 884, 902, 913, 924,
			888, 888, 914, 904, 915, 901, 886, 910,
			907, 907, 913, 921, 901, 877, 898, 885,
		};
		constexpr Score endgame_queen_table[SQUARE_COUNT] = {
			909, 930, 936, 932, 945, 941, 930, 951,
			903, 899, 893, 917, 924, 925, 920, 946,
			919, 892, 866, 915, 917, 920, 922, 960,
			943, 906, 877, 869, 897, 906, 953, 976,
			926, 906, 883, 873, 877, 896, 933, 961,
			921, 874, 873, 872, 866, 889, 911, 934,
			916, 886, 869, 874, 874, 878, 885, 907,
			908, 902, 889, 891, 910, 897, 903, 896,
		};
		constexpr Score opening_king_table[SQUARE_COUNT] = {
			-4, 2, 3, 1, -4, 0, 2, 0,
			3, 8, 5, 10, 3, 8, 5, -6,
			6, 13, 18, -1, 5, 22, 23, 0,
			0, 5, 10, -3, -6, 5, 12, -11,
			-12, 6, -7, -34, -37, -21, -9, -26,
			1, 11, -20, -38, -51, -36, 3, -3,
			8, 29, -8, -74, -58, -21, 36, 59,
			-22, 75, 35, -65, 30, -21, 69, 63,
		};
		constexpr Score endgame_king_table[SQUARE_COUNT] = {
			-29, -21, -15, -13, -10, 4, -3, -12,
			-12, 19, 26, 26, 26, 43, 22, -3,
			-6, 22, 31, 27, 30, 59, 52, -10,
			-30, 22, 31, 37, 28, 34, 20, -28,
			-43, -3, 26, 29, 31, 19, 1, -40,
			-37, 1, 17, 26, 26, 23, 11, -33,
			-45, -1, 16, 18, 23, 17, 5, -40,
			-95, -55, -33, -27, -32, -27, -45, -101,
		};
		constexpr Score opening_passed_pawn_table[SQUARE_COUNT] = {
			0, 0, 0, 0, 0, 0, 0, 0,
			36, 42, 30, 42, 30, 37, 9, -3,
			40, 25, 18, 17, 18, 28, 5, -6,
			9, 10, 6, -7, 0, 19, -4, -7,
			0, -24, -33, -35, -27, -16, -9, 13,
			-12, -27, -24, -32, -18, 2, -16, 16,
			-18, -6, 1, -15, -5, -9, -6, -15,
			0, 0, 0, 0, 0, 0, 0, 0,
		};
		constexpr Score endgame_passed_pawn_table[SQUARE_COUNT] = {
			0, 0, 0, 0, 0, 0, 0, 0,
			143, 138, 121, 109, 117, 114, 129, 141,
			166, 152, 111, 83, 70, 106, 116, 143,
			92, 76, 61, 49, 38, 50, 78, 76,
			46, 37, 25, 21, 16, 23, 42, 44,
			10, 12, 4, 4, 0, -2, 12, 10,
			0, 5, -5, 0, 0, -7, 0, 5,
			0, 0, 0, 0, 0, 0, 0, 0,
		};
		constexpr Score opening_doubled_pawn_table[SQUARE_COUNT] = {
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 1, 0, 0, 0, 0, 0,
			0, 2, 0, -2, -4, 0, 0, 2,
			-1, 12, -1, 0, -25, 9, 6, -4,
			-11, -3, 0, -3, -7, 3, 9, -10,
			-33, 12, -5, -7, -9, -7, 2, -23,
			0, 0, 0, 0, 0, 0, 0, 0,
		};
		constexpr Score endgame_doubled_pawn_table[SQUARE_COUNT] = {
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 2, 3, 0, 0, 2, 0, 3,
			3, -5, 2, 2, -6, -9, 6, -3,
			-22, -8, -24, -16, -7, -25, -8, -14,
			-31, -7, -20, -15, -8, -16, -11, -35,
			-41, -27, -29, -27, -42, -22, -19, -26,
			0, 0, 0, 0, 0, 0, 0, 0,
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
				_square_values[COLOR_BLACK][mirror_rank(square)][piece] = value;
			}
		}
		for (Square square = 0; square < SQUARE_COUNT; square++)
		{
			Value value = make_value(opening_passed_pawn_table[square], endgame_passed_pawn_table[square]);
			_passed_pawn_values[COLOR_WHITE][square] = value;
			_passed_pawn_values[COLOR_BLACK][mirror_rank(square)] = value;
		}
		for (Square square = 0; square < SQUARE_COUNT; square++)
		{
			Value value = make_value(opening_doubled_pawn_table[square], endgame_doubled_pawn_table[square]);
			_doubled_pawn_values[COLOR_WHITE][square] = value;
			_doubled_pawn_values[COLOR_BLACK][mirror_rank(square)] = value;
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
	constexpr static Score _weights[PIECE_COUNT] = { -92, 156, 240, 389, 999, -100 };
	constexpr static Score _total_weight = 3466;

	Value _move_values[PIECE_COUNT] = {};
	Value _attack_values[PIECE_COUNT] = {};
	Value _defense_values[PIECE_COUNT] = {};
	Value _square_values[COLOR_COUNT][SQUARE_COUNT][PIECE_COUNT] = {};
	Value _passed_pawn_values[COLOR_COUNT][SQUARE_COUNT] = {};
	Value _doubled_pawn_values[COLOR_COUNT][SQUARE_COUNT] = {};
	Value _castling_value_wk = {};
	Value _castling_value_wq = {};
	Value _castling_value_bk = {};
	Value _castling_value_bq = {};
};

inline const Evaluator evaluator;
