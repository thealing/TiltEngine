#pragma once

#include "pieces.hpp"

#include <string.h>

#include <memory>
#include <array>

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

	inline int get_move_value(const Move& move)
	{
		return get_move_value_reference(move);
	}

	inline void add_move_value(const Move& move, int delta)
	{
		int& value = get_move_value_reference(move);
		value += delta;
		value -= std::abs(delta) * value / 4000;
	}

	inline int& get_move_value_reference(const Move& move)
	{
		return _values[size_t(move.src_square)][size_t(move.dst_square)][move.captured_piece + 1];
	}

private:
	int _values[SQUARE_COUNT][SQUARE_COUNT][PIECE_COUNT + 1];
};

inline HistoryHeuristic history_heuristic;
