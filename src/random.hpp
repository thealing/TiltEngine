#pragma once

#include <stdint.h>

class Random
{
public:
	constexpr Random(uint64_t seed) : _seed(seed)
	{
	}

	constexpr uint64_t next()
	{
		_seed ^= _seed << 7;
		_seed ^= _seed >> 9;
		return _seed;
	}

private:
	uint64_t _seed;
};
