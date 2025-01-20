#pragma once

#include "bitboards.hpp"

#include <array>
#include <algorithm>

class Bitmasks
{
public:
	inline constexpr Bitboard get_knight_mask(Square square) const
	{
		return _knight_masks[square];
	}

	inline constexpr Bitboard get_bishop_mask(Square square) const
	{
		return _bishop_masks[square];
	}

	inline constexpr Bitboard get_rook_mask(Square square) const
	{
		return _rook_masks[square];
	}

	inline constexpr Bitboard get_king_mask(Square square) const
	{
		return _king_masks[square];
	}

	inline constexpr Bitmasks()
	{
		for (Rank src_rank = RANK_8; src_rank <= RANK_1; src_rank++)
		{
			for (File src_file = FILE_A; src_file <= FILE_H; src_file++)
			{
				for (Rank dst_rank = RANK_8; dst_rank <= RANK_1; dst_rank++)
				{
					for (File dst_file = FILE_A; dst_file <= FILE_H; dst_file++)
					{
						if (src_rank == dst_rank && src_file == dst_file)
						{
							continue;
						}
						Square src_square = make_square(src_rank, src_file);
						Square dst_square = make_square(dst_rank, dst_file);
						Rank rank_diff = std::max<Rank>(src_rank - dst_rank, dst_rank - src_rank);
						File file_diff = std::max<File>(src_file - dst_file, dst_file - src_file);
						if ((rank_diff == 1 && file_diff == 2) || (rank_diff == 2 && file_diff == 1))
						{
							set_square(_knight_masks[src_square], dst_square);
						}
						if (rank_diff == file_diff)
						{
							set_square(_bishop_masks[src_square], dst_square);
						}
						if (rank_diff == 0 || file_diff == 0)
						{
							set_square(_rook_masks[src_square], dst_square);
						}
						if (rank_diff <= 1 && file_diff <= 1)
						{
							set_square(_king_masks[src_square], dst_square);
						}
					}
				}
			}
		}
	}

private:
	Bitboard _knight_masks[SQUARE_COUNT] = {};
	Bitboard _bishop_masks[SQUARE_COUNT] = {};
	Bitboard _rook_masks[SQUARE_COUNT] = {};
	Bitboard _king_masks[SQUARE_COUNT] = {};
};

inline const Bitmasks bitmasks;
