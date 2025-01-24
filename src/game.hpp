#pragma once

#include "position.hpp"
#include "hasher.hpp"
#include "evaluator.hpp"

struct Game
{
	static constexpr char START_FEN[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	static constexpr int MAX_PLY = 1024;
	static constexpr int MAX_MOVES = 256;
	static constexpr int MAX_DEPTH = 64;

	Game()
	{
		set_fen(START_FEN);
	}

	void set_fen(const char fen[])
	{
		_position_stack[0].set_fen(fen);
		_hash_stack[0] = hasher.get_position_hash(_position_stack[0]);
		evaluator.evaluate_position(&_evaluation_stack[0], _position_stack[0]);
		_ply = 0;
	}

	Square parse_square(const char str[]) const
	{
		return _position_stack[_ply].parse_square(str);
	}

	Move parse_move(const char str[]) const
	{
		return _position_stack[_ply].parse_move(str);
	}

	inline const Position& get_current_position() const
	{
		return _position_stack[_ply];
	}

	inline Color get_current_color() const
	{
		return _position_stack[_ply].current_color;
	}

	inline Move* generate_moves(Move* move) const
	{
		return _position_stack[_ply].generate_moves(move);
	}

	inline Move* generate_captures(Move* move) const
	{
		return _position_stack[_ply].generate_captures(move);
	}

	inline bool play_move(const Move& move)
	{
		switch (get_current_color())
		{
			case COLOR_WHITE:
				return play_move<COLOR_WHITE>(move);
			case COLOR_BLACK:
				return play_move<COLOR_BLACK>(move);
			default:
				return false;
		}
	}

	template<Color color>
	inline bool play_move(const Move& move)
	{
		if (_ply + 1 == MAX_PLY)
		{
			return false;
		}
		Position& current_position = _position_stack[_ply];
		Position& next_position = _position_stack[_ply + 1];
		if (!current_position.play_move<color>(&next_position, move))
		{
			return false;
		}
		_hash_stack[_ply + 1] = _hash_stack[_ply] ^ hasher.get_move_hash<color>(move, current_position, next_position);
		evaluator.update_evaluation<color>(&_evaluation_stack[_ply + 1], &_evaluation_stack[_ply], move);
		_ply++;
		return true;
	}

	inline void undo_move()
	{
		_ply--;
	}

	inline int64_t perft(int depth)
	{
		depth = std::min(depth, MAX_DEPTH);
		Move* moves = new Move[MAX_MOVES * size_t(depth)];
		int64_t result = perft(depth, moves);
		delete[] moves;
		return result;
	}

	inline int64_t perft(int depth, Move* moves)
	{
		switch (get_current_color())
		{
			case COLOR_WHITE:
				return perft<COLOR_WHITE>(depth, moves);
			case COLOR_BLACK:
				return perft<COLOR_BLACK>(depth, moves);
			default:
				return false;
		}
	}

	template<Color color>
	inline int64_t perft(int depth, Move* moves)
	{
		constexpr Color enemy = flip_color(color);
		int64_t result = 0;
		Position& current_position = _position_stack[_ply];
		Position& next_position = _position_stack[_ply + 1];
		_ply++;
		Move* end = current_position.generate_moves<color, false>(moves);
		for (Move* move = moves; move != end; move++)
		{
			if (!current_position.play_move<color>(&next_position, *move))
			{
				continue;
			}
			if (depth == 1)
			{
				result++;
			}
			else
			{
				result += perft<enemy>(depth - 1, end);
			}
		}
		_ply--;
		return result;
	}

protected:
	inline bool is_draw_by_fifty_move_rule() const
	{
		return _position_stack[_ply].halfmove_clock >= 100;
	}

	inline bool is_draw_by_repetition() const
	{
		return std::count(_hash_stack, _hash_stack + _ply, _hash_stack[_ply]) >= 2;
	}

protected:
	Position _position_stack[(size_t)MAX_PLY];
	Hash _hash_stack[(size_t)MAX_PLY];
	Evaluation _evaluation_stack[(size_t)MAX_PLY];
	int _ply;
};
