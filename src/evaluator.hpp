#pragma once

#include "position.hpp"
#include "values.hpp"

#include <numeric>

struct Evaluation
{
	Value value;
	Score weight;
};

class Evaluator
{
public:
	inline constexpr void evaluate_position(Evaluation* evaluation, const Position& position) const
	{
		evaluation->value = get_position_value(position);
		evaluation->weight = get_position_weight(position);
	}

	template<Color color>
	inline constexpr void update_evaluation(Evaluation* dst_evaluation, const Evaluation* src_evaluation, const Move& move) const
	{
		dst_evaluation->value = src_evaluation->value + get_move_value<color>(move);
		dst_evaluation->weight = src_evaluation->weight + get_move_weight<color>(move);
	}

	inline constexpr Score get_evaluation_score(const Evaluation& evaluation) const
	{
		return interpolate_phases(evaluation.value, evaluation.weight);
	}

private:
	inline constexpr Value get_position_value(const Position& position) const
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
		return value;
	}

	inline constexpr Score get_position_weight(const Position& position) const
	{
		Score weight = 0;
		for (Piece piece = 0; piece < PIECE_COUNT; piece++)
		{
			Bitboard mask = position.pieces[piece];
			weight += _weights[piece] * count_squares(mask);
		}
		return weight;
	}

	inline constexpr Score interpolate_phases(Value value, Score weight) const
	{
		return (get_opening_score(value) * weight + get_endgame_score(value) * (_total_weight - weight)) / _total_weight;
	}

	template<Color color>
	inline constexpr Value get_move_value(const Move& move) const
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
	inline constexpr Score get_move_weight(const Move& move) const
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
		constexpr Score opening_move_values[PIECE_COUNT] = { -76604, 40365, 44908, -232784, 48274, 24748 };
		constexpr Score endgame_move_values[PIECE_COUNT] = { 175056, -4365, 260068, 746669, 315657, -77908 };
		constexpr Score opening_attack_values[PIECE_COUNT] = { 5573, -17144, -5949, -16468, -5384, -50310 };
		constexpr Score endgame_attack_values[PIECE_COUNT] = { 21777, 24248, 39631, 52813, 5716, 73541 };
		for (Piece piece = 0; piece < PIECE_COUNT; piece++)
		{
			_move_values[piece] = make_value(opening_move_values[piece], endgame_move_values[piece]);
			_attack_values[piece] = make_value(opening_attack_values[piece], endgame_attack_values[piece]);
		}
		constexpr Score opening_pawn_table[SQUARE_COUNT] = {
			100, 100, 100, 100, 100, 100, 100, 100,
			228, 266, 213, 260, 227, 258, 194, 119,
			137, 147, 167, 166, 205, 219, 176, 113,
			88, 125, 119, 135, 133, 120, 129, 73,
			64, 101, 95, 118, 122, 107, 113, 70,
			67, 97, 96, 88, 105, 108, 145, 86,
			57, 104, 76, 70, 82, 136, 152, 75,
			100, 100, 100, 100, 100, 100, 100, 100,
		};
		constexpr Score endgame_pawn_table[SQUARE_COUNT] = {
			100, 100, 100, 100, 100, 100, 100, 100,
			268, 299, 247, 280, 256, 279, 234, 180,
			165, 175, 187, 183, 213, 222, 195, 142,
			108, 138, 129, 141, 138, 128, 139, 91,
			84, 113, 105, 124, 128, 114, 122, 84,
			84, 110, 105, 100, 115, 115, 148, 96,
			77, 116, 91, 85, 97, 140, 155, 87,
			100, 100, 100, 100, 100, 100, 100, 100,
		};
		constexpr Score opening_knight_table[SQUARE_COUNT] = {
			182, 260, 295, 295, 330, 273, 278, 246,
			254, 291, 431, 381, 370, 423, 346, 328,
			286, 409, 403, 430, 450, 474, 449, 370,
			344, 389, 381, 442, 422, 443, 397, 384,
			345, 352, 383, 389, 407, 395, 386, 354,
			332, 352, 385, 376, 388, 395, 396, 343,
			313, 280, 345, 362, 365, 388, 335, 349,
			236, 336, 277, 312, 340, 322, 342, 303,
		};
		constexpr Score endgame_knight_table[SQUARE_COUNT] = {
			215, 290, 328, 322, 357, 301, 303, 262,
			288, 326, 442, 404, 394, 434, 369, 346,
			317, 425, 426, 449, 464, 483, 457, 386,
			368, 412, 410, 462, 444, 461, 418, 403,
			369, 378, 410, 417, 430, 420, 409, 376,
			357, 378, 406, 403, 412, 414, 412, 366,
			337, 313, 371, 387, 389, 404, 359, 366,
			271, 356, 309, 341, 363, 349, 361, 322,
		};
		constexpr Score opening_bishop_table[SQUARE_COUNT] = {
			383, 370, 294, 319, 336, 324, 343, 371,
			360, 418, 378, 359, 421, 431, 432, 350,
			384, 418, 446, 444, 415, 431, 433, 407,
			387, 412, 420, 459, 451, 451, 413, 401,
			387, 421, 423, 434, 446, 420, 416, 405,
			400, 424, 420, 428, 423, 443, 422, 410,
			396, 424, 422, 403, 416, 423, 446, 393,
			351, 393, 385, 378, 383, 388, 339, 368,
		};
		constexpr Score endgame_bishop_table[SQUARE_COUNT] = {
			404, 395, 332, 354, 369, 358, 374, 393,
			388, 438, 407, 387, 442, 449, 449, 376,
			410, 440, 464, 461, 439, 454, 453, 430,
			413, 436, 444, 477, 471, 470, 435, 425,
			411, 442, 447, 458, 465, 443, 437, 426,
			422, 443, 443, 450, 446, 461, 442, 429,
			417, 440, 442, 426, 438, 442, 460, 412,
			376, 416, 405, 403, 407, 410, 371, 392,
		};
		constexpr Score opening_rook_table[SQUARE_COUNT] = {
			546, 544, 529, 565, 562, 521, 525, 534,
			546, 552, 557, 570, 566, 574, 541, 543,
			502, 523, 527, 539, 525, 550, 567, 533,
			490, 500, 516, 544, 539, 547, 519, 505,
			484, 492, 507, 521, 530, 523, 539, 508,
			474, 499, 509, 506, 532, 528, 537, 492,
			466, 514, 500, 513, 528, 541, 526, 440,
			503, 512, 532, 545, 545, 530, 492, 500,
		};
		constexpr Score endgame_rook_table[SQUARE_COUNT] = {
			611, 609, 599, 628, 626, 590, 592, 600,
			610, 616, 622, 632, 626, 634, 606, 607,
			573, 591, 595, 604, 592, 610, 625, 596,
			561, 570, 586, 606, 603, 609, 583, 573,
			555, 562, 576, 587, 593, 585, 598, 570,
			544, 566, 574, 572, 593, 588, 595, 556,
			537, 577, 568, 579, 589, 600, 586, 514,
			568, 578, 596, 606, 605, 591, 558, 562,
		};
		constexpr Score opening_queen_table[SQUARE_COUNT] = {
			906, 922, 945, 931, 990, 948, 939, 950,
			917, 898, 938, 949, 944, 991, 975, 982,
			918, 928, 963, 957, 987, 1008, 993, 1003,
			903, 909, 930, 940, 950, 972, 945, 942,
			922, 912, 934, 937, 940, 941, 951, 937,
			911, 941, 927, 935, 935, 943, 957, 942,
			889, 922, 951, 936, 945, 951, 923, 918,
			920, 907, 922, 950, 908, 894, 892, 876,
		};
		constexpr Score endgame_queen_table[SQUARE_COUNT] = {
			1056, 1081, 1101, 1091, 1138, 1104, 1095, 1107,
			1060, 1056, 1095, 1107, 1104, 1137, 1120, 1122,
			1065, 1076, 1105, 1115, 1139, 1152, 1133, 1139,
			1057, 1066, 1084, 1097, 1110, 1123, 1105, 1095,
			1067, 1070, 1085, 1095, 1096, 1096, 1103, 1088,
			1060, 1079, 1079, 1084, 1084, 1093, 1102, 1088,
			1040, 1067, 1091, 1082, 1090, 1092, 1066, 1065,
			1066, 1053, 1066, 1085, 1061, 1041, 1040, 1016,
		};
		constexpr Score opening_king_table[SQUARE_COUNT] = {
			-21, 10, 11, 3, -29, -6, 16, 9,
			18, 24, -3, 20, -5, 9, 7, -25,
			26, 10, 33, -37, -18, 30, 36, 2,
			-1, -17, -10, -45, -42, -12, 12, -8,
			-19, -7, -46, -73, -72, -27, -2, -20,
			15, 23, -26, -53, -44, -25, 18, 10,
			37, 44, 6, -62, -43, 3, 53, 62,
			11, 87, 48, -59, 35, -14, 74, 63,
		};
		constexpr Score endgame_king_table[SQUARE_COUNT] = {
			-37, -1, 5, -3, -27, -2, 14, 2,
			11, 24, 0, 20, 0, 16, 11, -17,
			22, 14, 33, -25, -9, 36, 41, 4,
			-4, -8, -2, -30, -28, -1, 16, -7,
			-23, -6, -32, -54, -54, -18, -1, -21,
			6, 17, -19, -40, -32, -18, 16, 4,
			22, 33, 6, -51, -34, 3, 42, 46,
			-6, 63, 34, -54, 22, -17, 55, 39,
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
	constexpr static Score _weights[PIECE_COUNT] = { -116, 150, 212, 261, 665, -795 };
	constexpr static Score _total_weight = 376;

	Value _move_values[PIECE_COUNT] = {};
	Value _attack_values[PIECE_COUNT] = {};
	Value _square_values[COLOR_COUNT][SQUARE_COUNT][PIECE_COUNT] = {};
	Value _castling_value_wk = {};
	Value _castling_value_wq = {};
	Value _castling_value_bk = {};
	Value _castling_value_bq = {};
};

inline const Evaluator evaluator;
