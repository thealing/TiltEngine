#pragma once

#include "profiler.hpp"
#include "position.hpp"

#include <vector>

struct PawnStructureEntry
{
	Bitboard pawn_masks[COLOR_COUNT];
	Bitboard move_masks[COLOR_COUNT];
	Bitboard attack_masks[COLOR_COUNT];
	Bitboard passed_masks[COLOR_COUNT];
	Bitboard doubled_masks[COLOR_COUNT];

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
		pawn_masks[color] = pawn_mask;
		move_masks[color] = shift_forward<color>(pawn_mask);
		attack_masks[color] = shift_forward_left<color>(pawn_mask) | shift_forward_right<color>(pawn_mask);
	}

	template<Color color>
	inline void init_extra(const Position& position)
	{
		constexpr Color enemy = flip_color(color);
		Bitboard behind_mask = shift_backward<color>(pawn_masks[color]);
		behind_mask |= shift_backward<color, 1>(behind_mask);
		behind_mask |= shift_backward<color, 2>(behind_mask);
		behind_mask |= shift_backward<color, 4>(behind_mask);
		Bitboard enemy_span_mask = pawn_masks[enemy] | attack_masks[enemy];
		enemy_span_mask |= shift_forward<enemy, 1>(enemy_span_mask);
		enemy_span_mask |= shift_forward<enemy, 2>(enemy_span_mask);
		enemy_span_mask |= shift_forward<enemy, 4>(enemy_span_mask);
		passed_masks[color] = pawn_masks[color] & ~behind_mask & ~enemy_span_mask;
		doubled_masks[color] = pawn_masks[color] & behind_mask;
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
