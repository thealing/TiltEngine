#pragma once

#include "pieces.hpp"

#include <string.h>

class HistoryHeuristic
{
public:
	HistoryHeuristic()
	{
		clear();
	}

	inline void clear()
	{
		memset(_values, 0, sizeof(_values));
	}

	inline int get_value(const Move& move)
	{
		return get_reference(move);
	}

	inline void add_value(const Move& move, int delta)
	{
		int& value = get_reference(move);
		value += delta;
		value -= std::abs(delta) * value / 4000;
	}

	inline int& get_reference(const Move& move)
	{
		return _values[move.src_square][move.dst_square][move.captured_piece + 1];
	}

private:
	int _values[SQUARE_COUNT][SQUARE_COUNT][PIECE_COUNT + 1];
};

inline HistoryHeuristic history_heuristic;
