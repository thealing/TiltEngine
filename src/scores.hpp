#pragma once

#include <stdint.h>

using Score = int32_t;

inline constexpr Score SCORE_MAX = +200000;
inline constexpr Score SCORE_MIN = -200000;
inline constexpr Score SCORE_MATE = 100000;
inline constexpr Score SCORE_DRAW = 0;
