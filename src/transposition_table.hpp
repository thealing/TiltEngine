#pragma once

#include "move.hpp"
#include "hash.hpp"

#include <vector>

enum ScoreType : int8_t
{
	SCORE_TYPE_LOWER = 1,
	SCORE_TYPE_UPPER = 2,
	SCORE_TYPE_EXACT = 3
};

struct TranspositionEntry
{
	uint64_t hash;
	Score score;
	uint16_t move;
	int8_t type;
	int8_t depth;
};

class TranspositionTable
{
public:
	static constexpr int DEFAULT_SIZE = 1 << 22;

	TranspositionTable(size_t size = DEFAULT_SIZE) : _and(size - 1), _entries(size)
	{
		clear();
	}

	inline void clear()
	{
		std::fill(_entries.begin(), _entries.end(), TranspositionEntry{});
	}

	inline TranspositionEntry& get_entry(Hash hash)
	{
		return _entries[hash & _and];
	}

private:
	Hash _and;
	std::vector<TranspositionEntry> _entries;
};

inline TranspositionTable transposition_table;
