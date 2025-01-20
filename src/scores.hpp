#pragma once

#include <stdint.h>

using Score = int32_t;

inline constexpr Score SCORE_MIN = -30000;
inline constexpr Score SCORE_MAX = 30000;
inline constexpr Score SCORE_MATE = 28000;
inline constexpr Score SCORE_DRAW = 0;
