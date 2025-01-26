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
		value += _doubled_pawn_value * count_squares(entry.doubled_masks[color]);
		value += _isolated_pawn_value * count_squares(entry.isolated_masks[color]);
		value += _backward_pawn_value * count_squares(entry.backward_masks[color]);
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
			_opening_pawn_square_values,
			_opening_knight_square_values,
			_opening_bishop_square_values,
			_opening_rook_square_values,
			_opening_queen_square_values,
			_opening_king_square_values,
		};
		const Score* endgame_piece_tables[] = {
			_endgame_pawn_square_values,
			_endgame_knight_square_values,
			_endgame_bishop_square_values,
			_endgame_rook_square_values,
			_endgame_queen_square_values,
			_endgame_king_square_values,
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
			Value value = make_value(_opening_passed_pawn_values[square], _endgame_passed_pawn_values[square]);
			_passed_pawn_values[COLOR_WHITE][square] = value;
			_passed_pawn_values[COLOR_BLACK][mirror_rank(square)] = value;
		}
		_doubled_pawn_value = make_value(_opening_doubled_pawn_value, _endgame_doubled_pawn_value);
		_isolated_pawn_value = make_value(_opening_isolated_pawn_value, _endgame_isolated_pawn_value);
		_backward_pawn_value = make_value(_opening_backward_pawn_value, _endgame_backward_pawn_value);
	}

private:
	Value _move_values[PIECE_COUNT] = {};
	Value _attack_values[PIECE_COUNT] = {};
	Value _defense_values[PIECE_COUNT] = {};
	Value _square_values[COLOR_COUNT][SQUARE_COUNT][PIECE_COUNT] = {};
	Value _passed_pawn_values[COLOR_COUNT][SQUARE_COUNT] = {};
	Value _doubled_pawn_value = {};
	Value _isolated_pawn_value = {};
	Value _backward_pawn_value = {};

private:
	constexpr static Score _weights[PIECE_COUNT] = { -84, 136, 229, 372, 1008, -117 };
	constexpr static Score _total_weight = 3386;
	constexpr static Score _opening_move_values[PIECE_COUNT] = {
		14,   19,   16,    9,    6,    3
	};
	constexpr static Score _endgame_move_values[PIECE_COUNT] = {
		16,   15,   13,   14,   28,   -9
	};
	constexpr static Score _opening_attack_values[PIECE_COUNT] = {
		64,   19,   29,   24,    4,  -56
	};
	constexpr static Score _endgame_attack_values[PIECE_COUNT] = {
		16,   30,   43,   52,   66,   24
	};
	constexpr static Score _opening_defense_values[PIECE_COUNT] = {
		11,   16,   19,    9,    6,   16
	};
	constexpr static Score _endgame_defense_values[PIECE_COUNT] = {
		8,   15,   16,   32,   54,   -5
	};
	constexpr static Score _opening_pawn_square_values[SQUARE_COUNT] = {
		100,  100,  100,  100,  100,  100,  100,  100, 
		136,  142,  131,  144,  130,  138,  105,   89, 
		88,   89,   94,  104,  130,  156,  130,   94, 
		62,   69,   77,   95,   85,   90,   75,   57, 
		56,   56,   80,   92,  102,   90,   71,   56, 
		55,   61,   72,   78,   93,   87,  107,   68, 
		67,   74,   79,   83,   84,  126,  122,   76, 
		100,  100,  100,  100,  100,  100,  100,  100, 
	};
	constexpr static Score _endgame_pawn_square_values[SQUARE_COUNT] = {
		100,  100,  100,  100,  100,  100,  100,  100, 
		245,  239,  222,  210,  220,  216,  232,  243, 
		140,  145,  154,  144,  152,  144,  161,  141, 
		114,  115,  110,  107,  114,  116,  117,  110, 
		103,  105,  102,  100,  105,  100,  101,   93, 
		92,   96,   97,  105,  107,  102,   91,   87, 
		101,   98,  109,  107,  118,  108,   96,   85, 
		100,  100,  100,  100,  100,  100,  100,  100, 
	};
	constexpr static Score _opening_knight_square_values[SQUARE_COUNT] = {
		247,  286,  291,  296,  310,  279,  295,  283, 
		261,  269,  375,  311,  313,  331,  310,  301, 
		272,  323,  307,  325,  354,  367,  358,  326, 
		313,  325,  289,  370,  343,  356,  343,  352, 
		318,  307,  303,  317,  336,  327,  340,  340, 
		304,  288,  299,  288,  309,  310,  334,  312, 
		304,  281,  284,  320,  314,  321,  306,  330, 
		278,  322,  266,  288,  323,  285,  327,  297, 
	};
	constexpr static Score _endgame_knight_square_values[SQUARE_COUNT] = {
		274,  293,  320,  298,  319,  292,  286,  267, 
		306,  325,  310,  331,  325,  310,  319,  296, 
		301,  310,  309,  314,  311,  311,  308,  304, 
		326,  316,  321,  329,  323,  315,  314,  329, 
		325,  312,  314,  328,  323,  318,  324,  330, 
		322,  313,  292,  311,  310,  292,  298,  319, 
		313,  311,  306,  315,  316,  304,  315,  302, 
		305,  317,  310,  323,  321,  317,  312,  288, 
	};
	constexpr static Score _opening_bishop_square_values[SQUARE_COUNT] = {
		322,  307,  277,  289,  298,  287,  304,  317, 
		298,  318,  278,  289,  321,  322,  318,  266, 
		336,  318,  337,  317,  310,  333,  342,  365, 
		323,  360,  315,  355,  328,  350,  358,  350, 
		342,  335,  348,  329,  367,  323,  343,  367, 
		364,  365,  346,  355,  348,  377,  352,  379, 
		350,  372,  363,  346,  358,  349,  395,  343, 
		320,  356,  369,  346,  350,  349,  302,  339, 
	};
	constexpr static Score _endgame_bishop_square_values[SQUARE_COUNT] = {
		329,  321,  317,  322,  327,  321,  325,  321, 
		340,  327,  329,  314,  327,  329,  325,  326, 
		344,  322,  314,  312,  316,  319,  326,  347, 
		347,  323,  320,  307,  306,  321,  318,  348, 
		345,  325,  321,  317,  307,  321,  323,  345, 
		346,  335,  330,  335,  333,  323,  330,  348, 
		345,  332,  335,  340,  343,  331,  332,  328, 
		332,  355,  355,  349,  346,  352,  340,  337, 
	};
	constexpr static Score _opening_rook_square_values[SQUARE_COUNT] = {
		515,  519,  498,  533,  528,  505,  508,  509, 
		508,  516,  526,  540,  536,  535,  506,  517, 
		480,  506,  513,  522,  514,  524,  538,  510, 
		470,  480,  497,  520,  521,  528,  499,  485, 
		459,  476,  495,  497,  521,  501,  521,  478, 
		461,  480,  499,  501,  517,  522,  509,  472, 
		462,  501,  496,  513,  531,  528,  515,  437, 
		496,  507,  526,  536,  543,  514,  487,  499, 
	};
	constexpr static Score _endgame_rook_square_values[SQUARE_COUNT] = {
		557,  557,  560,  559,  563,  561,  561,  552, 
		551,  553,  550,  546,  541,  552,  560,  553, 
		558,  560,  554,  559,  549,  544,  548,  547, 
		558,  550,  559,  547,  552,  546,  538,  558, 
		553,  546,  548,  539,  536,  527,  531,  540, 
		541,  538,  526,  527,  520,  514,  525,  528, 
		538,  534,  533,  535,  524,  519,  523,  535, 
		533,  541,  541,  537,  535,  530,  536,  520, 
	};
	constexpr static Score _opening_queen_square_values[SQUARE_COUNT] = {
		913,  912,  926,  917,  951,  935,  936,  960, 
		887,  848,  890,  907,  896,  936,  930,  970, 
		903,  887,  909,  905,  935,  949,  957,  992, 
		880,  879,  871,  881,  894,  924,  923,  927, 
		902,  868,  892,  874,  896,  892,  915,  919, 
		891,  909,  880,  894,  886,  904,  915,  926, 
		887,  889,  915,  905,  916,  904,  886,  910, 
		906,  908,  914,  922,  902,  877,  896,  882, 
	};
	constexpr static Score _endgame_queen_square_values[SQUARE_COUNT] = {
		911,  935,  941,  937,  949,  947,  935,  957, 
		907,  907,  894,  922,  931,  927,  922,  951, 
		926,  895,  862,  918,  919,  920,  923,  964, 
		954,  911,  878,  868,  898,  906,  960,  987, 
		932,  911,  883,  871,  873,  896,  936,  970, 
		925,  872,  871,  869,  862,  885,  910,  939, 
		921,  884,  867,  872,  871,  873,  883,  908, 
		910,  903,  887,  889,  911,  899,  904,  896, 
	};
	constexpr static Score _opening_king_square_values[SQUARE_COUNT] = {
		-5,    2,    4,    1,   -5,   -1,    2,    0, 
		4,   10,    6,   12,    2,    9,    5,   -8, 
		7,   15,   22,   -3,    5,   26,   28,    0, 
		0,    5,   11,   -6,   -9,    4,   15,  -13, 
		-13,    7,  -10,  -44,  -47,  -26,  -11,  -27, 
		3,   13,  -24,  -45,  -59,  -40,    2,    1, 
		15,   32,  -11,  -76,  -60,  -22,   35,   62, 
		-15,   79,   38,  -63,   32,  -17,   72,   68, 
	};
	constexpr static Score _endgame_king_square_values[SQUARE_COUNT] = {
		-39,  -27,  -20,  -18,  -14,    2,   -7,  -18, 
		-16,   22,   30,   30,   30,   48,   26,   -5, 
		-9,   25,   33,   29,   32,   61,   54,  -13, 
		-34,   24,   33,   38,   30,   35,   21,  -30, 
		-45,    0,   28,   31,   32,   20,    3,  -42, 
		-39,    3,   19,   27,   28,   24,   13,  -34, 
		-47,    0,   18,   20,   25,   19,    8,  -41, 
		-102,  -56,  -34,  -29,  -33,  -27,  -46, -103, 
	};
	constexpr static Score _opening_passed_pawn_values[SQUARE_COUNT] = {
		0,    0,    0,    0,    0,    0,    0,    0, 
		36,   42,   31,   44,   30,   38,    5,  -10, 
		44,   25,   19,   15,   18,   27,    1,  -12, 
		20,   19,   12,   -8,    3,   23,   -2,   -4, 
		10,  -14,  -30,  -34,  -27,  -16,   -4,   19, 
		0,  -17,  -22,  -37,  -17,    5,  -11,   21, 
		-5,    6,    6,  -17,   -4,   -3,    2,   -7, 
		0,    0,    0,    0,    0,    0,    0,    0, 
	};
	constexpr static Score _endgame_passed_pawn_values[SQUARE_COUNT] = {
		0,    0,    0,    0,    0,    0,    0,    0, 
		145,  139,  122,  110,  120,  116,  132,  143, 
		169,  154,  113,   84,   72,  106,  119,  146, 
		95,   78,   61,   45,   38,   52,   81,   79, 
		48,   40,   26,   19,   16,   25,   46,   46, 
		13,   15,    5,    1,   -2,   -1,   18,   13, 
		5,    9,   -5,    0,    1,   -5,    6,    8, 
		0,    0,    0,    0,    0,    0,    0,    0, 
	};
	constexpr static Score _opening_doubled_pawn_value = 0;
	constexpr static Score _endgame_doubled_pawn_value = -20;
	constexpr static Score _opening_isolated_pawn_value = -16;
	constexpr static Score _endgame_isolated_pawn_value = -7;
	constexpr static Score _opening_backward_pawn_value = -18;
	constexpr static Score _endgame_backward_pawn_value = -13;
};

inline const Evaluator evaluator;
