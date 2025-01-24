#pragma once

#include "position.hpp"
#include "values.hpp"
#include "pawn_structure_table.hpp"

#include <numeric>

class Evaluator
{
public:
	inline Score evaluate_position(const Position& position) const
	{
		Value value = get_position_value(position);
		Score weight = get_position_weight(position);
		return interpolate_phases(value, weight);
	}

private:
	inline Value get_position_value(const Position& position) const
	{
		const PawnStructureEntry& entry = pawn_structure_table.get_entry(position);
		return get_position_value<COLOR_WHITE>(position, entry) - get_position_value<COLOR_BLACK>(position, entry);
	}

	template<Color color>
	inline Value get_position_value(const Position& position, const PawnStructureEntry& entry) const
	{
		Value value = 0;
		value += get_piece_square_value<color>(position);
		value += get_mobility_value<color>(position, entry);
		value += get_pawns_value<color>(position, entry);
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
	inline Value get_mobility_value(const Position& position, const PawnStructureEntry& entry) const
	{
		constexpr Color enemy = flip_color(color);
		Value value = 0;
		Bitboard color_mask =  position.colors[color];
		Bitboard enemy_mask =  position.colors[enemy];
		Bitboard occupied_mask = color_mask | enemy_mask;
		Bitboard empty_mask = ~occupied_mask;
		Bitboard safe_mask = ~entry.attack_masks[enemy];
		value += _move_values[PIECE_PAWN] * count_squares(empty_mask & safe_mask & entry.move_masks[color]);
		value += _attack_values[PIECE_PAWN] * count_squares(enemy_mask & safe_mask & entry.attack_masks[color]);
		value += _defense_values[PIECE_PAWN] * count_squares(color_mask & safe_mask & entry.attack_masks[color]);
		//Profiler::get().enter();
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
				dst_mask &= safe_mask;
				value += _move_values[piece] * count_squares(dst_mask & empty_mask);
				value += _attack_values[piece] * count_squares(dst_mask & enemy_mask);
				value += _defense_values[piece] * count_squares(dst_mask & color_mask);
			}
		}
		//Profiler::get().leave();
		return value;
	}

	template<Color color>
	inline Value get_pawns_value(const Position& position, const PawnStructureEntry& entry) const
	{
		Value value = 0;
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

public:
	inline constexpr Evaluator()
	{
		const Score* opening_piece_tables[] = {
			_opening_pawn_table,
			_opening_knight_table,
			_opening_bishop_table,
			_opening_rook_table,
			_opening_queen_table,
			_opening_king_table
		};
		const Score* endgame_piece_tables[] = {
			_endgame_pawn_table,
			_endgame_knight_table,
			_endgame_bishop_table,
			_endgame_rook_table,
			_endgame_queen_table,
			_endgame_king_table
		};
		for (Piece piece = 0; piece < PIECE_COUNT; piece++)
		{
			_move_values[piece] = make_value(_opening_move_values[piece], _endgame_move_values[piece]);
			_attack_values[piece] = make_value(_opening_attack_values[piece], _endgame_attack_values[piece]);
			_defense_values[piece] = make_value(_opening_defense_values[piece], _endgame_defense_values[piece]);
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
			Value value = make_value(_opening_passed_pawn_table[square], _endgame_passed_pawn_table[square]);
			_passed_pawn_values[COLOR_WHITE][square] = value;
			_passed_pawn_values[COLOR_BLACK][mirror_rank(square)] = value;
		}
		for (Square square = 0; square < SQUARE_COUNT; square++)
		{
			Value value = make_value(_opening_doubled_pawn_table[square], _endgame_doubled_pawn_table[square]);
			_doubled_pawn_values[COLOR_WHITE][square] = value;
			_doubled_pawn_values[COLOR_BLACK][mirror_rank(square)] = value;
		}
	}

private:
	Value _move_values[PIECE_COUNT] = {};
	Value _attack_values[PIECE_COUNT] = {};
	Value _defense_values[PIECE_COUNT] = {};
	Value _square_values[COLOR_COUNT][SQUARE_COUNT][PIECE_COUNT] = {};
	Value _passed_pawn_values[COLOR_COUNT][SQUARE_COUNT] = {};
	Value _doubled_pawn_values[COLOR_COUNT][SQUARE_COUNT] = {};

private:
	constexpr static Score _weights[PIECE_COUNT] = { -73, 93, 190, 304, 1020, -256 };
	constexpr static Score _total_weight = 2708;
	constexpr static Score _opening_move_values[PIECE_COUNT] = { 15, 19, 15, 11, 6, 10 };
	constexpr static Score _endgame_move_values[PIECE_COUNT] = { 16, 16, 12, 12, 30, -9 };
	constexpr static Score _opening_attack_values[PIECE_COUNT] = { 66, 18, 26, 29, 4, -52 };
	constexpr static Score _endgame_attack_values[PIECE_COUNT] = { 25, 29, 38, 43, 70, 14 };
	constexpr static Score _opening_defense_values[PIECE_COUNT] = { 13, 16, 15, 14, 5, 23 };
	constexpr static Score _endgame_defense_values[PIECE_COUNT] = { 10, 16, 13, 22, 63, -4 };
	constexpr static Score _opening_pawn_table[SQUARE_COUNT] = {
		100, 100, 100, 100, 100, 100, 100, 100,
		146, 149, 139, 157, 132, 149, 95, 71,
		95, 93, 94, 100, 137, 158, 133, 104,
		77, 84, 85, 98, 94, 94, 89, 66,
		68, 71, 86, 96, 109, 96, 87, 64,
		69, 77, 77, 82, 98, 96, 122, 77,
		82, 92, 85, 87, 91, 136, 140, 86,
		100, 100, 100, 100, 100, 100, 100, 100,
	};
	constexpr static Score _endgame_pawn_table[SQUARE_COUNT] = {
		100, 100, 100, 100, 100, 100, 100, 100,
		240, 236, 219, 209, 218, 214, 225, 232,
		141, 139, 145, 127, 146, 148, 156, 137,
		118, 119, 114, 109, 117, 122, 123, 112,
		106, 109, 106, 105, 111, 107, 108, 95,
		96, 101, 100, 105, 111, 107, 103, 91,
		107, 105, 112, 108, 124, 118, 110, 91,
		100, 100, 100, 100, 100, 100, 100, 100,
	};
	constexpr static Score _opening_knight_table[SQUARE_COUNT] = {
		210, 273, 280, 298, 319, 261, 293, 276,
		243, 261, 406, 324, 328, 353, 325, 307,
		265, 340, 322, 346, 383, 414, 389, 352,
		326, 341, 305, 385, 357, 379, 360, 371,
		332, 318, 320, 332, 352, 342, 361, 354,
		318, 303, 314, 304, 326, 325, 349, 327,
		317, 282, 300, 334, 328, 336, 317, 344,
		262, 337, 269, 304, 340, 302, 342, 306,
	};
	constexpr static Score _endgame_knight_table[SQUARE_COUNT] = {
		279, 305, 337, 313, 337, 306, 296, 260,
		318, 337, 338, 348, 343, 334, 340, 314,
		314, 332, 329, 336, 337, 338, 335, 326,
		343, 337, 337, 355, 347, 341, 337, 352,
		343, 330, 333, 347, 345, 340, 345, 350,
		339, 328, 312, 328, 330, 313, 321, 336,
		332, 327, 323, 334, 335, 324, 333, 322,
		323, 335, 324, 336, 340, 333, 332, 297,
	};
	constexpr static Score _opening_bishop_table[SQUARE_COUNT] = {
		351, 324, 261, 280, 301, 276, 308, 344,
		313, 352, 299, 293, 351, 355, 352, 285,
		361, 347, 377, 355, 343, 374, 382, 393,
		350, 386, 348, 393, 366, 391, 385, 378,
		374, 367, 379, 364, 401, 355, 376, 399,
		390, 396, 377, 386, 379, 408, 383, 404,
		386, 401, 395, 375, 388, 382, 423, 369,
		344, 387, 396, 377, 378, 375, 312, 366,
	};
	constexpr static Score _endgame_bishop_table[SQUARE_COUNT] = {
		359, 350, 342, 348, 354, 348, 355, 350,
		364, 360, 357, 345, 361, 362, 359, 351,
		373, 356, 353, 350, 352, 359, 363, 379,
		373, 362, 357, 352, 348, 361, 357, 377,
		375, 360, 360, 357, 351, 357, 359, 376,
		377, 370, 366, 370, 369, 364, 366, 381,
		378, 369, 371, 373, 377, 365, 372, 360,
		360, 384, 385, 379, 376, 380, 370, 369,
	};
	constexpr static Score _opening_rook_table[SQUARE_COUNT] = {
		520, 529, 496, 553, 548, 505, 506, 512,
		505, 517, 530, 556, 551, 549, 505, 522,
		478, 507, 515, 527, 520, 540, 558, 518,
		472, 482, 499, 529, 526, 543, 503, 488,
		468, 472, 498, 501, 526, 506, 532, 485,
		466, 481, 501, 503, 517, 525, 511, 476,
		468, 504, 498, 516, 533, 531, 521, 442,
		501, 512, 531, 541, 546, 520, 492, 503,
	};
	constexpr static Score _endgame_rook_table[SQUARE_COUNT] = {
		613, 613, 614, 617, 620, 615, 615, 608,
		606, 609, 609, 608, 602, 611, 614, 609,
		607, 612, 609, 614, 604, 601, 607, 601,
		605, 600, 612, 604, 608, 605, 593, 607,
		601, 599, 604, 596, 596, 586, 591, 591,
		592, 593, 586, 587, 583, 578, 586, 580,
		589, 591, 592, 596, 589, 584, 583, 582,
		589, 597, 601, 599, 598, 590, 588, 576,
	};
	constexpr static Score _opening_queen_table[SQUARE_COUNT] = {
		917, 912, 933, 923, 975, 955, 955, 974,
		883, 856, 900, 914, 895, 953, 945, 984,
		902, 891, 928, 910, 949, 971, 973, 997,
		873, 884, 879, 893, 903, 936, 923, 929,
		908, 870, 902, 882, 908, 900, 920, 920,
		894, 916, 889, 902, 897, 915, 924, 928,
		889, 896, 923, 913, 925, 916, 895, 919,
		911, 915, 925, 931, 910, 882, 901, 878,
	};
	constexpr static Score _endgame_queen_table[SQUARE_COUNT] = {
		926, 959, 963, 961, 960, 967, 946, 974,
		932, 930, 907, 941, 961, 935, 926, 966,
		955, 909, 849, 934, 926, 919, 923, 981,
		1001, 931, 886, 870, 907, 908, 994, 1029,
		955, 932, 885, 873, 869, 897, 952, 1006,
		947, 871, 867, 869, 855, 876, 907, 966,
		946, 889, 867, 870, 867, 861, 881, 919,
		923, 913, 884, 893, 918, 908, 916, 906,
	};
	constexpr static Score _opening_king_table[SQUARE_COUNT] = {
		-8, 11, 16, 7, -12, -2, 9, 4,
		12, 20, 7, 20, -2, 11, 5, -18,
		20, 25, 40, -12, 3, 43, 45, 3,
		8, 5, 18, -22, -24, 2, 25, -16,
		-14, 10, -26, -80, -83, -50, -20, -25,
		22, 16, -45, -67, -81, -54, -7, 18,
		45, 31, -22, -90, -71, -35, 24, 69,
		21, 85, 46, -55, 40, -6, 77, 85,
	};
	constexpr static Score _endgame_king_table[SQUARE_COUNT] = {
		-80, -49, -34, -30, -24, -1, -16, -42,
		-26, 31, 35, 37, 35, 51, 30, -10,
		-11, 31, 39, 31, 34, 64, 58, -12,
		-32, 27, 36, 37, 29, 35, 25, -28,
		-44, 4, 26, 23, 23, 16, 4, -40,
		-35, 9, 17, 22, 21, 20, 16, -29,
		-41, 8, 20, 13, 20, 19, 15, -27,
		-99, -38, -22, -32, -23, -24, -29, -83,
	};
	constexpr static Score _opening_passed_pawn_table[SQUARE_COUNT] = {
		0, 0, 0, 0, 0, 0, 0, 0,
		46, 49, 39, 57, 32, 49, -4, -28,
		59, 38, 32, 28, 17, 22, -7, -26,
		18, 16, 13, -4, 0, 27, -20, -8,
		4, -21, -28, -34, -31, -22, -19, 16,
		-11, -28, -24, -40, -22, 0, -32, 11,
		-18, -6, 4, -23, -13, -12, -14, -19,
		0, 0, 0, 0, 0, 0, 0, 0,
	};
	constexpr static Score _endgame_passed_pawn_table[SQUARE_COUNT] = {
		0, 0, 0, 0, 0, 0, 0, 0,
		140, 136, 119, 109, 118, 114, 125, 132,
		160, 150, 116, 98, 78, 105, 117, 136,
		85, 70, 56, 43, 34, 47, 68, 69,
		41, 31, 18, 14, 11, 17, 36, 42,
		8, 7, 1, 0, -3, -2, 9, 11,
		-1, 4, -5, 1, -1, -7, 1, 3,
		0, 0, 0, 0, 0, 0, 0, 0,
	};
	constexpr static Score _opening_doubled_pawn_table[SQUARE_COUNT] = {
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 3, 0, 0, 0, -2, 0,
		-2, 7, 0, -5, -13, 3, -1, 9,
		3, 25, 0, 2, -33, 20, 12, -4,
		-14, -6, -1, -3, -9, 3, 11, -8,
		-36, 8, -8, -6, -9, -10, 1, -24,
		0, 0, 0, 0, 0, 0, 0, 0,
	};
	constexpr static Score _endgame_doubled_pawn_table[SQUARE_COUNT] = {
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 6, 12, -3, -2, 3, -1, 8,
		8, -16, 2, 2, -19, -27, 5, -14,
		-29, -11, -29, -20, -13, -29, -13, -18,
		-32, -8, -20, -16, -10, -17, -11, -33,
		-43, -25, -28, -36, -50, -23, -18, -28,
		0, 0, 0, 0, 0, 0, 0, 0,
	};
};

inline const Evaluator evaluator;
