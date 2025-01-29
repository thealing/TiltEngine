#pragma once

#include "bitboards.hpp"
#include "random.hpp"

#include <string.h>

class Sliders
{
private:
	using Slider = int;

	enum : Slider
	{
		SLIDER_BISHOP,
		SLIDER_ROOK,
		SLIDER_COUNT
	};

	//class MagicGenerator
	//{
	//private:
	//	static constexpr int MAX_BITS = 6;

	//public:
	//	uint64_t next()
	//	{
	//		int i = 0;
	//		while (i < _n && _a[i] == 63 - i) {
	//			i++;
	//		}
	//		if (i == _n) {
	//			_n++;
	//			if (_n > MAX_BITS) {
	//				return 0;
	//			}
	//		}
	//		else {
	//			_a[i]++;
	//		}
	//		for (int j = i - 1; j >= 0; j--) {
	//			_a[j] = _a[j + 1] + 1;
	//		}
	//		uint64_t result = 0;
	//		for (int j = 0; j < _n; j++) {
	//			result |= 1ULL << _a[j];
	//		}
	//		return result;
	//	}

	//private:
	//	int _a[64] = {};
	//	int _n = 1;
	//};

public:
	static inline Bitboard get_bishop_mask(Square square, Bitboard occupied_mask)
	{
		return get_slider_mask(SLIDER_BISHOP, square, occupied_mask);
	}

	static inline Bitboard get_rook_mask(Square square, Bitboard occupied_mask)
	{
		return get_slider_mask(SLIDER_ROOK, square, occupied_mask);
	}

	static inline Bitboard get_queen_mask(Square square, Bitboard occupied_mask)
	{
		return get_bishop_mask(square, occupied_mask) | get_rook_mask(square, occupied_mask);
	}

private:
	static inline Bitboard get_slider_hash(Slider slider, int square, Bitboard value)
	{
		return value * MAGICS[slider][square] >> SHIFTS[slider];
	}

	static inline Bitboard get_slider_mask(Slider slider, Square square, Bitboard occupied_mask)
	{
		return _tables[slider][square][get_slider_hash(slider, square, occupied_mask & _masks[slider][square])];
	}

private:
	static bool init()
	{
		constexpr Bitboard edge_mask = get_rank_mask(RANK_1, RANK_8) | get_file_mask(FILE_A, FILE_H);
		constexpr Bitboard corner_mask = get_square_mask(SQUARE_A8, SQUARE_H8, SQUARE_A1, SQUARE_H1);
		for (Square square = 0; square < SQUARE_COUNT; square++) {
			_masks[SLIDER_BISHOP][square] = Bitboards::get_bishop_mask(square) & ~edge_mask;
			if (test_square(edge_mask, square)) {
				_masks[SLIDER_ROOK][square] = Bitboards::get_rook_mask(square) & ~corner_mask;
				if (get_square_rank(square) == RANK_1 || get_square_rank(square) == RANK_8) {
					_masks[SLIDER_ROOK][square] &= ~get_square_mask(flip_vertically(square));
				}
				if (get_square_file(square) == FILE_A || get_square_file(square) == FILE_H) {
					_masks[SLIDER_ROOK][square] &= ~get_square_mask(flip_horizontally(square));
				}
			}
			else {
				_masks[SLIDER_ROOK][square] = Bitboards::get_rook_mask(square) & ~edge_mask;
			}
		}
		for (Slider slider = 0; slider < SLIDER_COUNT; slider++) {
			size_t max_size = 1ULL << (64 - SHIFTS[slider]);
			Bitboard* table = new Bitboard[max_size * SQUARE_COUNT] {};
			int* collision_map = new int[max_size] {};
			int iteration = 0;
			for (Rank src_rank = 0; src_rank < RANK_COUNT; src_rank++) {
				for (File src_file = 0; src_file < FILE_COUNT; src_file++) {
					Square src_square = make_square(src_rank, src_file);
					Bitboard mask = _masks[slider][src_square];
					int count = count_squares(mask);
					size_t ideal_size = 1ULL << count;
					size_t best_size = max_size + 1;
					//MagicGenerator generator;
					//Bitboard best_magic = 0;
					Bitboard best_magic = MAGICS[slider][src_square];
					while (true) {
						iteration++;
						//Bitboard magic = generator.next();
						//if (magic == 0) {
						//	magic = best_magic;
						//}
						Bitboard magic = best_magic;
						Bitboard sub_mask = mask;
						size_t size = 0;
						bool good = true;
						while (true) {
							Bitboard hash = sub_mask * magic >> SHIFTS[slider];
							if (hash >= size) {
								size = hash + 1;
							}
							Bitboard& dst_mask = table[hash];
							if (collision_map[hash] == iteration) {
								good = false; 
								break;
							}
							collision_map[hash] = iteration;
							constexpr int direction_count = 4;
							constexpr Rank rank_deltas[SLIDER_COUNT][direction_count] = {
								{ -1, 1, 1, -1 },
								{ -1, 0, 1, 0 }
							};
							constexpr File file_deltas[SLIDER_COUNT][direction_count] = {
								{ 1, 1, -1, -1 },
								{ 0, 1, 0, -1 }
							};
							for (int direction = 0; direction < direction_count; direction++) {
								Rank dst_rank = src_rank;
								File dst_file = src_file;
								while (true) {
									dst_rank += rank_deltas[slider][direction];
									dst_file += file_deltas[slider][direction];
									if (dst_rank < 0 || dst_rank >= RANK_COUNT || dst_file < 0 || dst_file >= FILE_COUNT) {
										break;
									}
									Square dst_square = make_square(dst_rank, dst_file);
									set_square(dst_mask, dst_square);
									if (test_square(sub_mask, dst_square)) {
										break;
									}
								}
							}
							if (sub_mask == BITBOARD_EMPTY) {
								break;
							}
							sub_mask = (sub_mask - 1) & mask;
						}
						if (good) {
							if (magic == best_magic || size == ideal_size) {
								best_size = size;
								//printf("%2d  %2d  %4lld  %4lld\n", slider, src_square, ideal_size, best_size);
								//printf("0x%016llXLL, ", magic);
								//MAGICS[slider][src_square] = magic;
								break;
							}
							if (size < best_size) {
								best_size = size;
								best_magic = magic;
							}
						}
					}
					_tables[slider][src_square] = table;
					table += best_size;
				}
			}
			delete[] collision_map;
		}
		return true;
	}

private:
	static constexpr Bitboard MAGICS[SLIDER_COUNT][SQUARE_COUNT] = {
		{
			0x0008020080200802LL, 0x0002008020080200LL, 0x0001004008020000LL, 0x0000806004000000LL, 0x0000440200000000LL, 0x000021C100800000LL, 0x0000808080804000LL, 0x0000808080808040LL,
			0x0000040100401004LL, 0x0000020080200802LL, 0x0000010040080200LL, 0x0000008060040000LL, 0x0000004402000000LL, 0x00000021C1008000LL, 0x0000008080808040LL, 0x0000004040404020LL,
			0x0004000200802008LL, 0x0002000100401004LL, 0x0004000200200802LL, 0x0000200200801000LL, 0x0000240080840000LL, 0x0000100080440040LL, 0x0000080040404020LL, 0x0000040020202010LL,
			0x0002020001004010LL, 0x0001010000802008LL, 0x0002020001001004LL, 0x0002008008008002LL, 0x0000840000802000LL, 0x0000208000404020LL, 0x0000800800202010LL, 0x0000400400101008LL,
			0x0001010100008020LL, 0x0000808080004010LL, 0x0001010100008008LL, 0x0000020080080080LL, 0x0010020080001004LL, 0x0008020010002010LL, 0x0001004008001008LL, 0x0000802004000804LL,
			0x0000808080800040LL, 0x0000404040400020LL, 0x0000808080800040LL, 0x0000008840400020LL, 0x0000020041000010LL, 0x0010040100100008LL, 0x0002008020080004LL, 0x0001004010040002LL,
			0x0000808080804000LL, 0x0000404040402000LL, 0x0000001010806000LL, 0x0000000008403000LL, 0x0000000100202000LL, 0x0000040100200800LL, 0x0004010040100400LL, 0x0002008020080200LL,
			0x0000808080808040LL, 0x0000004040404020LL, 0x0000000010108060LL, 0x0000000000084030LL, 0x0000000001002020LL, 0x0000000401002008LL, 0x0000040100401004LL, 0x0008020080200802LL
		},
		{
			0x0080004000802010LL, 0x0020001000080020LL, 0x0040100008004004LL, 0x0040080004004002LL, 0x0040040002004001LL, 0x0020008020010202LL, 0x0040010000800040LL, 0x0080010000402080LL,
			0x0000081020410400LL, 0x0000100008040010LL, 0x0000080402010008LL, 0x0000200400020020LL, 0x0000200200010020LL, 0x0000200100008020LL, 0x0000400080004001LL, 0x0000200020004081LL,
			0x0040002000100020LL, 0x0004001000080010LL, 0x0004000802010008LL, 0x0002002004002002LL, 0x0001002002002001LL, 0x0001002000802001LL, 0x0000004040008001LL, 0x0000802000400020LL,
			0x0040200010080010LL, 0x0000080010040010LL, 0x0004020008010008LL, 0x0000040020020020LL, 0x0000020020010020LL, 0x0000010020008020LL, 0x0000400040008001LL, 0x0000200020004081LL,
			0x0040001000200020LL, 0x0000080400100010LL, 0x0004020100080008LL, 0x0000200400200200LL, 0x0000200200200100LL, 0x0000200100200080LL, 0x0000008000404001LL, 0x0000802000200040LL,
			0x0040201004000800LL, 0x0010080201000400LL, 0x0000400840804200LL, 0x0000040002002020LL, 0x0000020001002020LL, 0x0000010000802020LL, 0x0000400080004001LL, 0x0000100804010802LL,
			0x0040002000100020LL, 0x0000100008040010LL, 0x0004000802010008LL, 0x0000040020020020LL, 0x0000020020010020LL, 0x0000010020008020LL, 0x0000200040008020LL, 0x0000802000400020LL,
			0x0000800100402011LL, 0x0000400080201009LL, 0x0000200040100805LL, 0x0000402010080402LL, 0x0000080010040201LL, 0x0000048004080201LL, 0x0000024002040081LL, 0x0001000200804021LL
		}
	};
	static constexpr int SHIFTS[SLIDER_COUNT] = { 
		55, 
		52
	};

private:
	static inline Bitboard _masks[SLIDER_COUNT][SQUARE_COUNT];
	static inline Bitboard* _tables[SLIDER_COUNT][SQUARE_COUNT];

private:
	static inline const bool _init = init();
};
