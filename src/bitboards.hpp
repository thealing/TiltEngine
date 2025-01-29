#pragma once

#include "types.hpp"

#include <algorithm>

class Bitboards
{
public:
	static inline Bitboard get_knight_mask(Square square)
	{
		return _knight_masks[square];
	}

	static inline Bitboard get_bishop_mask(Square square)
	{
		return _bishop_masks[square];
	}

	static inline Bitboard get_rook_mask(Square square)
	{
		return _rook_masks[square];
	}

	static inline Bitboard get_king_mask(Square square)
	{
		return _king_masks[square];
	}

	static inline Bitboard get_check_mask(Square king_square, Square checker_square)
	{
		return _check_masks[king_square][checker_square];
	}

private:
	static bool init()
	{
		for (Rank src_rank = 0; src_rank < RANK_COUNT; src_rank++) {
			for (File src_file = 0; src_file < FILE_COUNT; src_file++) {
				for (Rank dst_rank = 0; dst_rank < RANK_COUNT; dst_rank++) {
					for (File dst_file = 0; dst_file < FILE_COUNT; dst_file++) {
						if (src_rank == dst_rank && src_file == dst_file) {
							continue;
						}
						Square src_square = make_square(src_rank, src_file);
						Square dst_square = make_square(dst_rank, dst_file);
						Rank rank_diff = std::max<Rank>(src_rank - dst_rank, dst_rank - src_rank);
						File file_diff = std::max<File>(src_file - dst_file, dst_file - src_file);
						if ((rank_diff == 1 && file_diff == 2) || (rank_diff == 2 && file_diff == 1)) {
							set_square(_knight_masks[src_square], dst_square);
							set_square(_check_masks[src_square][dst_square], dst_square);
						}
						if (rank_diff == file_diff) {
							set_square(_bishop_masks[src_square], dst_square);
						}
						if (rank_diff == 0 || file_diff == 0) {
							set_square(_rook_masks[src_square], dst_square);
						}
						if (rank_diff <= 1 && file_diff <= 1) {
							set_square(_king_masks[src_square], dst_square);
						}
						constexpr int direction_count = 8;
						constexpr Rank rank_deltas[direction_count] = { -1, -1, -1, 0, 0, 1, 1, 1 };
						constexpr Rank file_deltas[direction_count] = { -1, 0, 1, -1, 1, -1, 0, 1 };
						for (int direction = 0; direction < direction_count; direction++) {
							Rank rank = src_rank;
							File file = src_file;
							Square square = src_square;
							Bitboard check_mask = BITBOARD_EMPTY;
							bool reached = false;
							while (true) {
								if (!reached) {
									set_square(check_mask, square);
								}
								rank += rank_deltas[direction];
								file += file_deltas[direction];
								if (rank < 0 || rank >= RANK_COUNT || file < 0 || file >= FILE_COUNT) {
									break;
								}
								square = make_square(rank, file);
								if (square == dst_square) {
									reached = true;
								}
							}
							if (reached) {
								_check_masks[dst_square][src_square] = check_mask;
							}
						}
					}
				}
			}
		}
		return true;
	}

private:
	static inline Bitboard _knight_masks[SQUARE_COUNT];
	static inline Bitboard _bishop_masks[SQUARE_COUNT];
	static inline Bitboard _rook_masks[SQUARE_COUNT];
	static inline Bitboard _king_masks[SQUARE_COUNT];
	static inline Bitboard _check_masks[SQUARE_COUNT][SQUARE_COUNT];

private:
	static inline const bool _init = init();
};
