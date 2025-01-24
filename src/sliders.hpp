#pragma once

#include "bitmasks.hpp"

#include <algorithm>

class Sliders
{
public:
	inline constexpr Bitboard get_bishop_hash(int square, Bitboard value) const
	{
		return value * bishop_multipliers[square] >> BISHOP_SHIFT;
	}

	inline constexpr Bitboard get_rook_hash(int square, Bitboard value) const
	{
		return value * rook_multipliers[square] >> ROOK_SHIFT;
	}

	inline constexpr Bitboard get_bishop_mask(Square square, Bitboard occupied_mask) const
	{
		return _bishop_tables[square][get_bishop_hash(square, occupied_mask & _bishop_masks[square])];
	}

	inline constexpr Bitboard get_rook_mask(Square square, Bitboard occupied_mask) const
	{
		return _rook_tables[square][get_rook_hash(square, occupied_mask & _rook_masks[square])];
	}

	inline constexpr Bitboard get_queen_mask(Square square, Bitboard occupied_mask) const
	{
		return get_bishop_mask(square, occupied_mask) | get_rook_mask(square, occupied_mask);
	}

	inline constexpr Sliders()
	{
		constexpr Bitboard edge_mask = get_rank_mask(RANK_1) | get_rank_mask(RANK_8) | get_file_mask(FILE_A) | get_file_mask(FILE_H);
		constexpr Bitboard corner_mask = get_square_mask(SQUARE_A1) | get_square_mask(SQUARE_H1) | get_square_mask(SQUARE_A8) | get_square_mask(SQUARE_H8);
		for (Square square = 0; square < SQUARE_COUNT; square++)
		{
			_bishop_masks[square] = bitmasks.get_bishop_mask(square) & ~edge_mask;
			if (test_square(edge_mask, square))
			{
				_rook_masks[square] = bitmasks.get_rook_mask(square) & ~corner_mask;
				if (get_square_rank(square) == RANK_1 || get_square_rank(square) == RANK_8)
				{
					_rook_masks[square] &= ~get_square_mask(mirror_rank(square));
				}
				if (get_square_file(square) == FILE_A || get_square_file(square) == FILE_H)
				{
					_rook_masks[square] &= ~get_square_mask(mirror_file(square));
				}
			}
			else
			{
				_rook_masks[square] = bitmasks.get_rook_mask(square) & ~edge_mask;
			}
		}
		for (Rank src_rank = RANK_8; src_rank <= RANK_1; src_rank++)
		{
			for (File src_file = FILE_A; src_file <= FILE_H; src_file++)
			{
				Square src_square = make_square(src_rank, src_file);
				Bitboard mask_mask = _bishop_masks[src_square];
				Bitboard sub_mask = mask_mask + 1;
				do
				{
					sub_mask = (sub_mask - 1) & mask_mask;
					Bitboard hash = get_bishop_hash(src_square, sub_mask);
					for (Rank dst_rank = src_rank + 1, dst_file = src_file + 1; dst_rank <= RANK_1 && dst_file <= FILE_H; dst_rank++, dst_file++)
					{
						Square square = make_square(dst_rank, dst_file);
						set_square(_bishop_tables[src_square][hash], square);
						if (test_square(sub_mask, square))
						{
							break;
						}
					}
					for (Rank dst_rank = src_rank + 1, dst_file = src_file - 1; dst_rank <= RANK_1 && dst_file >= FILE_A; dst_rank++, dst_file--)
					{
						Square square = make_square(dst_rank, dst_file);
						set_square(_bishop_tables[src_square][hash], square);
						if (test_square(sub_mask, square))
						{
							break;
						}
					}
					for (Rank dst_rank = src_rank - 1, dst_file = src_file - 1; dst_rank >= RANK_8 && dst_file >= FILE_A; dst_rank--, dst_file--)
					{
						Square square = make_square(dst_rank, dst_file);
						set_square(_bishop_tables[src_square][hash], square);
						if (test_square(sub_mask, square))
						{
							break;
						}
					}
					for (Rank dst_rank = src_rank - 1, dst_file = src_file + 1; dst_rank >= RANK_8 && dst_file <= FILE_H; dst_rank--, dst_file++)
					{
						Square square = make_square(dst_rank, dst_file);
						set_square(_bishop_tables[src_square][hash], square);
						if (test_square(sub_mask, square))
						{
							break;
						}
					}
				} 
				while (sub_mask != 0);
			}
		}
		for (Rank src_rank = RANK_8; src_rank <= RANK_1; src_rank++)
		{
			for (File src_file = FILE_A; src_file <= FILE_H; src_file++)
			{
				Square src_square = make_square(src_rank, src_file);
				Bitboard mask_mask = _rook_masks[src_square];
				Bitboard sub_mask = mask_mask + 1;
				do
				{
					sub_mask = (sub_mask - 1) & mask_mask;
					Bitboard hash = get_rook_hash(src_square, sub_mask);
					for (File dst_file = src_file + 1; dst_file <= FILE_H; dst_file++)
					{
						Square square = make_square(src_rank, dst_file);
						set_square(_rook_tables[src_square][hash], square);
						if (test_square(sub_mask, square))
						{
							break;
						}
					}
					for (File dst_file = src_file - 1; dst_file >= FILE_A; dst_file--)
					{
						Square square = make_square(src_rank, dst_file);
						set_square(_rook_tables[src_square][hash], square);
						if (test_square(sub_mask, square))
						{
							break;
						}
					}
					for (Rank dst_rank = src_rank + 1; dst_rank <= RANK_1; dst_rank++)
					{
						Square square = make_square(dst_rank, src_file);
						set_square(_rook_tables[src_square][hash], square);
						if (test_square(sub_mask, square))
						{
							break;
						}
					}
					for (Rank dst_rank = src_rank - 1; dst_rank >= RANK_8; dst_rank--)
					{
						Square square = make_square(dst_rank, src_file);
						set_square(_rook_tables[src_square][hash], square);
						if (test_square(sub_mask, square))
						{
							break;
						}
					}
				} 
				while (sub_mask != 0);
			}
		}
	}

private:
	static constexpr int BISHOP_HASH_BITS = 9;
	static constexpr int ROOK_HASH_BITS = 12;
	static constexpr int BISHOP_SHIFT = 64 - BISHOP_HASH_BITS;
	static constexpr int ROOK_SHIFT = 64 - ROOK_HASH_BITS;
	static constexpr int BISHOP_TABLE_SIZE = 1 << BISHOP_HASH_BITS;
	static constexpr int ROOK_TABLE_SIZE = 1 << ROOK_HASH_BITS;

	static constexpr Bitboard bishop_multipliers[SQUARE_COUNT] = {
		0x208A200214110024, 0x0008826802002000, 0x00080E1C00202584, 0x0102208203680004, 0x00220A1020449802, 0x0004884440101100, 0x00110090100A4250, 0x0043004202A04800,
		0x400028900C082042, 0x4082082801040020, 0x1062221204460880, 0x0441082042409000, 0x20802E0211800000, 0x0000221202208000, 0x3902140202026000, 0x0010004118011001,
		0x0140208450040108, 0x0050100212021402, 0x0848001000204113, 0x0805800802004104, 0x0414005E020A0408, 0x2201000201038A00, 0x0900400401441020, 0x0401040344088440,
		0x0020140130108200, 0x0110480C04018400, 0x0080680030004243, 0x0000802108020060, 0x0880840010802000, 0x2001020007024520, 0x0201004011041000, 0x0008420309010100,
		0x0C04044020200220, 0x0012014401200840, 0x000C241000010100, 0x0008200801110050, 0x000801004084004E, 0x0409046101520100, 0x0502080060090400, 0x000088B200008201,
		0x0808085411041C21, 0x00212C1004000200, 0x0102001048040410, 0x0000402011010800, 0x2022A83101400400, 0x0005101001401080, 0x0102040400912400, 0x1004008401400104,
		0x4641081206200810, 0x3101008630060008, 0x24000D1041101820, 0x4400100442020402, 0x10A028C0082A0A02, 0x0208400208224060, 0x2229200404104000, 0x31A014040160C002,
		0x40A2042504100400, 0x0044250048020840, 0x050004014200B000, 0x40400400088C0C00, 0x4430800108210900, 0x1001024008210704, 0x0204A08204280881, 0x0004018202040700
	};
	static constexpr Bitboard rook_multipliers[SQUARE_COUNT] = {
		0x4080008010204000, 0x0040014010002000, 0x0480100008802002, 0x0480080030000581, 0x0100028C10080100, 0x0100080A04000500, 0x0080060010800100, 0x4200010204412084,
		0x0104800020804002, 0x000A00220882C900, 0x0001801001802000, 0x02A0801000080182, 0x000A800800800400, 0x000C800200808400, 0x0001006402008100, 0x09F0800040800100,
		0x4008808000204000, 0x1010004001200040, 0x2091010020001148, 0x0800210010030008, 0x0488004004004200, 0x000180800A000400, 0x2000040010010822, 0x0000020010C08401,
		0x1280104440062000, 0x00A5008100214000, 0x0250002020080400, 0x0610890100201000, 0x0409003100060800, 0x0802005A00100804, 0x0422521400500128, 0x001011A200014304,
		0x0B10604000800480, 0x0010006000400940, 0x2010802000801000, 0x0100821800801000, 0x1810050801001100, 0x0401800400802200, 0x0800020104001008, 0x0403000043000092,
		0x0380204008848000, 0x4010002000C84003, 0x002100A000410014, 0x20212A0010420020, 0x0000110008010024, 0x0204440002008080, 0x0822000C08060083, 0x1012042240820001,
		0x0250210142018A00, 0x5200401002200240, 0x0810008020001080, 0x0042281001002100, 0x00208048004C0080, 0x0420060004008080, 0x4030020805100400, 0x1004210044008200,
		0x0309001048208001, 0x1800308440010065, 0x0008200040110319, 0x50110015100008A1, 0x080A00102008040A, 0x044E000450080102, 0x040200C210080104, 0x10480C0D02408222
	};

	Bitboard _bishop_masks[SQUARE_COUNT] = {};
	Bitboard _rook_masks[SQUARE_COUNT] = {};
	Bitboard _bishop_tables[SQUARE_COUNT][BISHOP_TABLE_SIZE] = {};
	Bitboard _rook_tables[SQUARE_COUNT][ROOK_TABLE_SIZE] = {};
};

inline const Sliders sliders;
