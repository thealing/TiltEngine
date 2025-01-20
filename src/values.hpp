#pragma once

#include "scores.hpp"
#include "position.hpp"

using Value = int64_t;

inline constexpr Value make_value(Score opening_score, Score endgame_score)
{
	return Value((uint64_t)(uint32_t)endgame_score << 32) + Value(opening_score);
}

inline constexpr Score get_opening_score(Value value)
{
	return Score(value);
}

inline constexpr Score get_endgame_score(Value value)
{
	return Score((value + (Value)0x80000000) >> 32);
}
