#pragma once

#include "position.hpp"

#include <vector>

struct PawnStructureEntry
{
	Bitboard pawn_masks[COLOR_COUNT];
	Bitboard move_masks[COLOR_COUNT];
	Bitboard attack_masks[COLOR_COUNT];
	Bitboard passed_masks[COLOR_COUNT];
	Bitboard doubled_masks[COLOR_COUNT];

	PawnStructureEntry() = default;

	PawnStructureEntry(const Position& position)
	{
		init(position);
	}

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
	static constexpr int DEFAULT_SIZE = 1 << 20;

	PawnStructureTable(size_t size = DEFAULT_SIZE) : _and(size - 1), _entries(size)
	{
		clear();
	}

	inline void clear()
	{
		std::fill(_entries.begin(), _entries.end(), PawnStructureEntry{});
	}

	inline PawnStructureEntry& get_entry(Bitboard pawns[COLOR_COUNT])
	{
		// TODO: better hash?
		uint64_t hash = pawns[COLOR_WHITE] * 123456789 + pawns[COLOR_BLACK] * 987654321;
		return _entries[hash & _and];
	}

private:
	uint64_t _and;
	std::vector<PawnStructureEntry> _entries;
};
