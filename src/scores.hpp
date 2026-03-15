#pragma once

#include <stdint.h>

using Score = int32_t;

inline constexpr Score SCORE_MAX = +2000000;
inline constexpr Score SCORE_MIN = -2000000;
inline constexpr Score SCORE_MATE = 1000000;
inline constexpr Score SCORE_DRAW = 0;
