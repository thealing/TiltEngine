#pragma once

#pragma region DEBUG

#if true//enable this pile of trash

#include <iostream>
#include <iomanip>
#include <string>
#include <thread>
#include <cmath>
using std::cout;
using std::endl;
using std::dec;
using std::hex;
#define T() clock()
#define SQ_STR(s) (std::string("") + char('a' + s % 8) + char('8' - s / 8))
#define TO_STRING(b) ([](Bitboard b) -> std::string { std::string s; while (b) s += SQ_STR(get_square(b)) + ' ', pop_square(b); return s; }(b))
#define TIME_BLOCK(x) int x=clock();for(int _i=0;_i<1;_i++,x=clock()-x)
class Stopwatch {
public:
	Stopwatch() { _start_time = get_time(); }
	inline int64_t get_elapsed_time() const { return get_time() - _start_time; }
	inline int64_t get_time() const {
		using namespace std::chrono;
		duration time = system_clock::now().time_since_epoch();
		return duration_cast<microseconds>(time).count();
	}
private:
	int64_t _start_time;
};
class Profiler {
	static const int64_t REPORT_INTERVAL = 1000000;
public:
	static Profiler& get() {
		static Profiler instance;
		return instance;
	}
public:
	Profiler() : _block(false), _thread(&Profiler::procedure, this) {}
	~Profiler() { _thread.detach(); }
	void enter() { _block = true; }
	void leave() { _block = false; }
private:
	void procedure() {
		while (true) {
			int total_count = 0;
			int active_count = 0;
			Stopwatch stopwatch;
			while (stopwatch.get_elapsed_time() < REPORT_INTERVAL) {
				wait(999);
				if (_block) {
					active_count++;
				}
				total_count++;
			}
			cout << "USAGE: " << std::lround(100.0 * active_count / total_count) << '%' << std::endl;
		}
	}
	void wait(int64_t duration) {
		Stopwatch stopwatch;
		while (stopwatch.get_elapsed_time() < duration) {}
	}
private:
	bool _block;
	std::thread _thread;
};

#if true//enable profiler
#define PE() Profiler::get().enter()
#define PL() Profiler::get().leave()
#else
#define PE()
#define PL()
#endif

#endif

#pragma endregion

#include <bit>

#include <stdint.h>
#include <math.h>

using Color = int8_t;

using Piece = int8_t;

using Rank = int8_t;

using File = int8_t;

using Square = int8_t;

using Bitboard = uint64_t;

using Score = int32_t;

using Value = int64_t;

enum : Color
{
	COLOR_WHITE,
	COLOR_BLACK,
	COLOR_COUNT
};

enum : Piece
{
	PIECE_NONE = -1,
	PIECE_PAWN,
	PIECE_KNIGHT,
	PIECE_BISHOP,
	PIECE_ROOK,
	PIECE_QUEEN,
	PIECE_KING,
	PIECE_COUNT
};

enum : Rank
{
	RANK_8,
	RANK_7,
	RANK_6,
	RANK_5,
	RANK_4,
	RANK_3,
	RANK_2,
	RANK_1,
	RANK_COUNT
};

enum : File
{
	FILE_A,
	FILE_B,
	FILE_C,
	FILE_D,
	FILE_E,
	FILE_F,
	FILE_G,
	FILE_H,
	FILE_COUNT
};

enum : Square
{
	SQUARE_NONE = -1,
	SQUARE_A8, SQUARE_B8, SQUARE_C8, SQUARE_D8, SQUARE_E8, SQUARE_F8, SQUARE_G8, SQUARE_H8,
	SQUARE_A7, SQUARE_B7, SQUARE_C7, SQUARE_D7, SQUARE_E7, SQUARE_F7, SQUARE_G7, SQUARE_H7,
	SQUARE_A6, SQUARE_B6, SQUARE_C6, SQUARE_D6, SQUARE_E6, SQUARE_F6, SQUARE_G6, SQUARE_H6,
	SQUARE_A5, SQUARE_B5, SQUARE_C5, SQUARE_D5, SQUARE_E5, SQUARE_F5, SQUARE_G5, SQUARE_H5,
	SQUARE_A4, SQUARE_B4, SQUARE_C4, SQUARE_D4, SQUARE_E4, SQUARE_F4, SQUARE_G4, SQUARE_H4,
	SQUARE_A3, SQUARE_B3, SQUARE_C3, SQUARE_D3, SQUARE_E3, SQUARE_F3, SQUARE_G3, SQUARE_H3,
	SQUARE_A2, SQUARE_B2, SQUARE_C2, SQUARE_D2, SQUARE_E2, SQUARE_F2, SQUARE_G2, SQUARE_H2,
	SQUARE_A1, SQUARE_B1, SQUARE_C1, SQUARE_D1, SQUARE_E1, SQUARE_F1, SQUARE_G1, SQUARE_H1,
	SQUARE_COUNT
};

enum : Bitboard
{
	BITBOARD_EMPTY = 0,
	BITBOARD_FULL = UINT64_MAX
};

enum : Score
{
	SCORE_MAX = +30000,
	SCORE_MIN = -30000,
	SCORE_MATE = 28000,
	SCORE_DRAW = 0
};

constexpr Color flip_color(Color color)
{
	return color ^ 1;
}

constexpr Square make_square(Rank rank, File file)
{
	return Square(rank * 8 + file);
}

constexpr Rank get_square_rank(Square square)
{
	return square / 8;
}

constexpr File get_square_file(Square square)
{
	return square % 8;
}

constexpr Square flip_vertically(Square square)
{
	return square ^ 56;
}

constexpr Square flip_horizontally(Square square)
{
	return square ^ 7;
}

constexpr Bitboard get_square_mask(Square square)
{
	return 1ULL << square;
}

template<typename... Squares>
constexpr Bitboard get_square_mask(Square square, Squares... squares)
{
	return get_square_mask(square) | get_square_mask(squares...);
}

constexpr Bitboard get_rank_mask(Rank rank)
{
	return 0x00000000000000FFULL << rank * 8;
}

template<typename... Ranks>
constexpr Bitboard get_rank_mask(Rank rank, Ranks... ranks)
{
	return get_rank_mask(rank) | get_rank_mask(ranks...);
}

constexpr Bitboard get_file_mask(File file)
{
	return 0x0101010101010101ULL << file;
}

template<typename... Ranks>
constexpr Bitboard get_file_mask(Rank file, Ranks... files)
{
	return get_file_mask(file) | get_file_mask(files...);
}

constexpr bool test_square(Bitboard bitboard, Square square)
{
	return bitboard & get_square_mask(square);
}

constexpr void set_square(Bitboard& bitboard, Square square)
{
	bitboard |= get_square_mask(square);
}

constexpr void reset_square(Bitboard& bitboard, Square square)
{
	bitboard &= ~get_square_mask(square);
}

constexpr Square get_square(Bitboard bitboard)
{
	return Square(std::countr_zero(bitboard));
}

constexpr Square pop_square(Bitboard& bitboard)
{
	Square square = get_square(bitboard);
	bitboard &= bitboard - 1;
	return square;
}

constexpr int count_squares(Bitboard bitboard)
{
	return std::popcount(bitboard);
}

constexpr bool has_multiple_squares(Bitboard bitboard)
{
	return bitboard & (bitboard - 1);
}

template<Color color>
inline constexpr Rank get_promotion_rank()
{
	if constexpr (color == COLOR_WHITE) {
		return RANK_8;
	}
	if constexpr (color == COLOR_BLACK) {
		return RANK_1;
	}
}

template<Color color>
inline constexpr Rank get_starting_rank()
{
	if constexpr (color == COLOR_WHITE) {
		return RANK_2;
	}
	if constexpr (color == COLOR_BLACK) {
		return RANK_7;
	}
}

template<Color color, int Amount = 1>
inline constexpr Square move_forward(Square square)
{
	if constexpr (color == COLOR_WHITE) {
		return square - 8 * Amount;
	}
	if constexpr (color == COLOR_BLACK) {
		return square + 8 * Amount;
	}
}

template<Color color, int Amount = 1>
inline constexpr Square move_backward(Square square)
{
	if constexpr (color == COLOR_WHITE) {
		return square + 8 * Amount;
	}
	if constexpr (color == COLOR_BLACK) {
		return square - 8 * Amount;
	}
}

template<Color color>
inline constexpr Square move_backward_left(Square square)
{
	if constexpr (color == COLOR_WHITE) {
		return square + 7;
	}
	if constexpr (color == COLOR_BLACK) {
		return square - 9;
	}
}

template<Color color>
inline constexpr Square move_backward_right(Square square)
{
	if constexpr (color == COLOR_WHITE) {
		return square + 9;
	}
	if constexpr (color == COLOR_BLACK) {
		return square - 7;
	}
}

template<Color color, int Amount = 1>
inline constexpr Bitboard shift_forward(Bitboard bitboard)
{
	if constexpr (color == COLOR_WHITE) {
		return bitboard >> 8 * Amount;
	}
	if constexpr (color == COLOR_BLACK) {
		return bitboard << 8 * Amount;
	}
}

template<Color color, int Amount = 1>
inline constexpr Bitboard shift_backward(Bitboard bitboard)
{
	if constexpr (color == COLOR_WHITE) {
		return bitboard << 8 * Amount;
	}
	if constexpr (color == COLOR_BLACK) {
		return bitboard >> 8 * Amount;
	}
}

template<Color color>
inline constexpr Bitboard shift_forward_left(Bitboard bitboard)
{
	if constexpr (color == COLOR_WHITE) {
		return bitboard >> 9;
	}
	if constexpr (color == COLOR_BLACK) {
		return bitboard << 7;
	}
}

template<Color color>
inline constexpr Bitboard shift_forward_right(Bitboard bitboard)
{
	if constexpr (color == COLOR_WHITE) {
		return bitboard >> 7;
	}
	if constexpr (color == COLOR_BLACK) {
		return bitboard << 9;
	}
}

constexpr Value make_value(Score opening_score, Score endgame_score)
{
	return (Value(endgame_score) << 32) + Value(opening_score);
}

constexpr Score get_opening_score(Value value)
{
	return Score(value);
}

constexpr Score get_endgame_score(Value value)
{
	return Score((value + 0x80000000) >> 32);
}

inline void format_rank(std::ostream& os, Rank rank)
{
	os << char('8' - rank);
}

inline void format_file(std::ostream& os, File file)
{
	os << char('a' + file);
}

inline void format_square(std::ostream& os, Square square)
{
	format_file(os, get_square_file(square));
	format_rank(os, get_square_rank(square));
}
