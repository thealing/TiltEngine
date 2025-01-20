#pragma once

#include <stdint.h>

#include <chrono>

using Time = int64_t;

inline constexpr Time TIME_MAX = INT64_MAX;

inline Time get_time()
{
	using namespace std::chrono;
	duration time = system_clock::now().time_since_epoch();
	return duration_cast<milliseconds>(time).count();
}
