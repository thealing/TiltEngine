#pragma once

#include "position.hpp"

#include <string.h>

struct PawnStructureEntry
{
	Bitboard pawn_masks[COLOR_COUNT];
	Bitboard move_masks[COLOR_COUNT];
	Bitboard move_span_masks[COLOR_COUNT];
	Bitboard attack_masks[COLOR_COUNT];
	Bitboard attack_span_masks[COLOR_COUNT];
	Bitboard passed_masks[COLOR_COUNT];
	Bitboard doubled_masks[COLOR_COUNT];
	Bitboard isolated_masks[COLOR_COUNT];
	Bitboard backward_masks[COLOR_COUNT];

	inline void init(const Position& position)
	{
		init_basic<COLOR_WHITE>(position);
		init_basic<COLOR_BLACK>(position);
		init_extra<COLOR_WHITE>(position);
		init_extra<COLOR_BLACK>(position);
	}

	template<Color color>
	inline void init_basic(const Position& position)
	{
		Bitboard pawn_mask = position.get_mask(PIECE_PAWN, color);
		Bitboard move_mask = shift_forward<color>(pawn_mask);
		Bitboard attack_mask = shift_forward_left<color>(pawn_mask) | shift_forward_right<color>(pawn_mask);
		pawn_masks[color] = pawn_mask;
		move_masks[color] = move_mask;
		move_span_masks[color] = span_forward<color>(move_mask);
		attack_masks[color] = attack_mask;
		attack_span_masks[color] = span_forward<color>(attack_mask);
	}

	template<Color color>
	inline void init_extra(const Position& position)
	{
		constexpr Color enemy = flip_color(color);
		Bitboard pawn_mask = pawn_masks[color];
		Bitboard behind_mask = shift_backward<color>(pawn_mask);
		behind_mask = span_backward<color>(behind_mask);
		Bitboard controlled_mask = move_span_masks[enemy] | attack_span_masks[enemy];
		Bitboard adjacent_mask = shift_left(pawn_mask) | shift_right(pawn_mask);
		adjacent_mask = span_forward<color>(adjacent_mask) | span_backward<color>(adjacent_mask);
		Bitboard stopped_mask = attack_masks[enemy] & ~attack_span_masks[color];
		stopped_mask = shift_backward<color>(stopped_mask);
		stopped_mask = span_backward<color>(stopped_mask);
		passed_masks[color] = pawn_mask & ~behind_mask & ~controlled_mask;
		doubled_masks[color] = pawn_mask & behind_mask;
		isolated_masks[color] = pawn_mask & ~adjacent_mask & controlled_mask & ~behind_mask;
		backward_masks[color] = pawn_mask & adjacent_mask & stopped_mask & ~shift_backward<color>(position.pieces[PIECE_PAWN]);
	}
};

class PawnStructureTable
{
public:
	PawnStructureTable()
	{
		clear();
	}

	inline void clear()
	{
		memset(_entries, 0, sizeof(_entries));
	}

	inline const PawnStructureEntry& get_entry(const Position& position)
	{
		Bitboard white_pawn_mask = position.get_mask(PIECE_PAWN, COLOR_WHITE);
		Bitboard black_pawn_mask = position.get_mask(PIECE_PAWN, COLOR_BLACK);
		uint64_t hash = (white_pawn_mask * WHITE_MULTIPLIER + black_pawn_mask * BLACK_MULTIPLIER) >> SHIFT;
		PawnStructureEntry& entry = _entries[hash];
		if (entry.pawn_masks[COLOR_WHITE] != white_pawn_mask || entry.pawn_masks[COLOR_BLACK] != black_pawn_mask)
		{
			entry.init(position);
		}
		return entry;
	}

private:
	static constexpr int HASH_BITS = 12;
	static constexpr int SIZE = 1 << HASH_BITS;
	static constexpr int SHIFT = 64 - HASH_BITS;

	static constexpr Hash WHITE_MULTIPLIER = 0x9E3779B97F4A7C15;
	static constexpr Hash BLACK_MULTIPLIER = 0xC13FA9A902A6328F;

	PawnStructureEntry _entries[SIZE];
};

inline PawnStructureTable pawn_structure_table;
