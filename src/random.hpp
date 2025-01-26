#pragma once

#include <stdint.h>

class Random
{
public:
	constexpr Random() : _seed(0)
	{
	}

	constexpr Random(uint64_t seed) : _seed(seed)
	{
	}

	constexpr uint64_t next() 
	{
		_seed *= 2862933555777941757ULL;
		_seed += 3037000493ULL;
		return _seed;
	}

private:
	uint64_t _seed;
};
