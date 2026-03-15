#pragma once

#include "game.hpp"
#include "time.hpp"
#include "transposition_table.hpp"
#include "history_heuristic.hpp"

#include <math.h>

#include <iostream>
#include <fstream>
#include <string>
#include <thread>

inline std::ostream& operator<<(std::ostream& os, const Square& square)
{
	return os << char('a' + get_square_file(square)) << char('8' - get_square_rank(square));
}

inline std::ostream& operator<<(std::ostream& os, const Move& move)
{
	os << move.src_square << move.dst_square;
	switch (move.type)
	{
		case MOVE_TYPE_PROMOTION_Q:
			os << 'q';
			break;
		case MOVE_TYPE_PROMOTION_R:
			os << 'r';
			break;
		case MOVE_TYPE_PROMOTION_B:
			os << 'b';
			break;
		case MOVE_TYPE_PROMOTION_N:
			os << 'n';
			break;
	}
	return os;
}

class Search : public Game
{
public:
	Search()
	{
		new_game();
		reset();
	}

	inline void new_game()
	{
		set_fen(START_FEN);
		_score = 0;
		transposition_table.clear();
		history_heuristic.clear();
		pawn_structure_table.clear();
	}

	inline void start(int depth_limit, int64_t node_limit, Time time_limit)
	{
		reset();
		if (depth_limit != 0)
		{
			_depth_limit = depth_limit;
		}
		if (node_limit != 0)
		{
			_node_limit = node_limit;
		}
		if (time_limit != 0)
		{
			_stop_time = _start_time + time_limit;
		}
		start_thread();
	}

	inline void start(const Time times[COLOR_COUNT], const Time increments[COLOR_COUNT], int moves_to_go)
	{
		reset();
		Color color = get_current_color();
		Time time_left = times[color];
		Time increment = increments[color];
		int remaining_moves = int(45 - atan(abs(_score) * 0.006) * 20);
		if (moves_to_go)
		{
			remaining_moves = std::min(remaining_moves, moves_to_go);
		}
		_stop_time = _start_time + time_left / 5;
		_think_time = (time_left + increment * remaining_moves) / remaining_moves;
		start_thread();
	}

	inline void start_thread()
	{
		_thread = std::thread(&Search::think, this);
	}

	inline void stop()
	{
		_node_limit = 0;
		if (_thread.joinable())
		{
			_thread.join();
		}
	}

	inline void think()
	{
		_node_count = 0;
		_score = 0;
		_depth = 0;
		_move = Move{};
		for (int depth = 1; depth <= _depth_limit; depth++)
		{
			int window = 40;
			int iterations = 0;
			Score new_score = _score;
			while (can_continue())
			{
				iterations++;
				Score min_score = new_score - window;
				Score max_score = new_score + window;
				new_score = search(depth, min_score, max_score);
				if (new_score <= min_score || new_score >= max_score)
				{
					window *= 2;
				}
				else
				{
					break;
				}
			}
			if (!can_continue())
			{
				break;
			}
			_score = new_score;
			_depth = depth;
			_move = get_current_position().extract_move(transposition_table.get_entry(_hash_stack[_ply]).move);
			print_info();
			if (_think_time != 0 && get_time() - _start_time >= _think_time * iterations)
			{
				break;
			}
		}
		print_move();
	}

	inline Score search(int depth, Score alpha, Score beta)
	{
		switch (_position_stack[_ply].current_color)
		{
			case COLOR_WHITE:
				return search<COLOR_WHITE, false>(0, depth, alpha, beta);
			case COLOR_BLACK:
				return search<COLOR_BLACK, false>(0, depth, alpha, beta);
			default:
				return 0;
		}
	}

	template<Color color, bool quiescence>
	inline Score search(int current_depth, int remaining_depth, Score alpha, Score beta)
	{
		_node_count++;
		Hash hash = _hash_stack[_ply];
		const Position& position = _position_stack[_ply];
		if constexpr (!quiescence)
		{
			if (current_depth > 0)
			{
				if (position.halfmove_clock >= 100)
				{
					return SCORE_DRAW;
				}
				for (int i = _ply - position.halfmove_clock; i < _ply; i++)
				{
					if (_hash_stack[i] == hash)
					{
						return SCORE_DRAW;
					}
				}
			}
		}
		Score position_score;
		position_score = evaluator.evaluate_position(position);
		if constexpr (color == COLOR_BLACK)
		{
			position_score = -position_score;
		}
		if constexpr (quiescence)
		{
			if (position_score > alpha)
			{
				if (position_score >= beta)
				{
					return position_score;
				}
				alpha = position_score;
			}
		}
		TranspositionEntry& entry = transposition_table.get_entry(hash);
		bool hit = entry.hash == hash;
		bool lower;
		bool upper;
		Score entry_score;
		if (hit)
		{
			lower = (entry.type & SCORE_TYPE_LOWER) != 0;
			upper = (entry.type & SCORE_TYPE_UPPER) != 0;
			entry_score = entry.score;
			if (entry_score >= SCORE_MATE - MAX_PLY)
			{
				entry_score -= current_depth;
			}
			else if (entry_score <= -SCORE_MATE + MAX_PLY)
			{
				entry_score += current_depth;
			}
			if (entry.depth >= remaining_depth && ((entry_score <= alpha && upper) || (entry_score >= beta && lower)))
			{
				return entry_score;
			}
		}
		Score current_score = position_score;
		if (hit && (entry_score >= current_score ? lower : upper))
		{
			current_score = entry_score;
		}
		if (current_depth == MAX_DEPTH)
		{
			return current_score;
		}
		bool in_check = position.is_in_check<color>();
		if (!quiescence && current_depth > 0 && !in_check)
		{
			Score razor_margin = remaining_depth * 200;
			if (remaining_depth <= 3 && current_score + razor_margin <= alpha)
			{
				if (remaining_depth <= 1)
				{
					return search<color, true>(current_depth, 0, alpha, alpha + 1);
				}
				alpha -= razor_margin;
				Score score = search<color, true>(current_depth, 0, alpha, alpha + 1);
				if (score <= alpha)
				{
					return score;
				}
				alpha += razor_margin;
			}
			Score futility_margin = remaining_depth * 150;
			if (remaining_depth <= 8 && current_score - futility_margin >= beta)
			{
				Score score = search<color, true>(current_depth, 0, alpha, beta);
				if (score >= beta && abs(score) < SCORE_MATE - MAX_DEPTH)
				{
					return score;
				}
			}
		}
		Move entry_move = position.extract_move(entry.move);
		Score best_score = quiescence ? current_score : SCORE_MIN;
		ScoreType score_type = SCORE_TYPE_UPPER;
		Move best_move = {};
		Move moves[MAX_MOVES];
		int move_scores[MAX_MOVES];
		Move* moves_end = position.generate_moves<color, quiescence>(moves);
		int move_count = int(moves_end - moves);
		int legal_move_count = 0;
		int quiet_move_count = 0;
		constexpr int piece_scores[] = { 100, 300, 350, 500, 900 };
		for (int i = 0; i < move_count; i++)
		{
			if (hit && moves[i] == entry_move)
			{
				move_scores[i] = 9000000;
			}
			else if (moves[i].captured_piece != PIECE_NONE)
			{
				Piece piece = moves[i].get_moved_piece();
				if (_ply >= 1 && moves[i].dst_square == _move_stack[_ply - 1].dst_square)
				{
					move_scores[i] = 8000000 - piece_scores[piece];
				}
				else
				{
					move_scores[i] = 7000000 - piece_scores[piece] + piece_scores[moves[i].captured_piece];
				}
			}
			else
			{
				move_scores[i] = 6000000;
			}
			move_scores[i] += history_heuristic.get_value(moves[i]);
		}
		for (int i = 0; i < move_count; i++)
		{
			int bi = i;
			for (int j = i + 1; j < move_count; j++)
			{
				if (move_scores[j] > move_scores[bi])
				{
					bi = j;
				}
			}
			Move move = moves[bi];
			moves[bi] = moves[i];
			move_scores[bi] = move_scores[i];
			moves[i] = move;
			if (!play_move<color>(move))
			{
				move_scores[i] = -1;
				continue;
			}
			_mm_prefetch((const char*)&transposition_table.get_entry(_hash_stack[_ply]), _MM_HINT_NTA);
			legal_move_count++;
			if (move.captured_piece == PIECE_NONE)
			{
				quiet_move_count++;
			}
			Score score = 0;
			int reduction = -1;
			if (legal_move_count > 1)
			{
				reduction = legal_move_count / 10 + remaining_depth / 5;
				score = -search_next<color, quiescence>(current_depth + 1, remaining_depth - reduction - 1, -alpha - 1, -alpha);
				if (score > alpha)
				{
					reduction = -1;
				}
			}
			if (reduction < 0)
			{
				score = -search_next<color, quiescence>(current_depth + 1, remaining_depth - 1, -beta, -alpha);
			}
			undo_move();
			if (!can_continue())
			{
				return SCORE_MIN;
			}
			if (score > best_score)
			{
				best_score = score;
				best_move = move;
			}
			if (score > alpha)
			{
				alpha = score;
				score_type = SCORE_TYPE_EXACT;
			}
			if (score >= beta)
			{
				int delta = remaining_depth * remaining_depth;
				history_heuristic.add_value(move, delta);
				for (int j = 0; j < i; j++)
				{
					if (move_scores[j] == -1)
					{
						continue;
					}
					history_heuristic.add_value(moves[j], -delta);
				}
				score_type = SCORE_TYPE_LOWER;
				break;
			}
			if (!quiescence && !in_check)
			{
				if (legal_move_count >= remaining_depth * 7 && alpha == beta - 1)
				{
					break;
				}
			}
		}
		if (legal_move_count == 0)
		{
			if constexpr (quiescence)
			{
				best_score = current_score;
			}
			else
			{
				best_score = in_check ? current_depth - SCORE_MATE : SCORE_DRAW;
			}
			score_type = SCORE_TYPE_EXACT;
		}
		entry_score = best_score;
		if (entry_score >= SCORE_MATE - MAX_PLY)
		{
			entry_score += current_depth;
		}
		else if (entry_score <= -SCORE_MATE + MAX_PLY)
		{
			entry_score -= current_depth;
		}
		entry = TranspositionEntry{ hash, entry_score, (uint16_t)best_move, score_type, int8_t(remaining_depth) };
		return best_score;
	}

	template<Color color, bool quiescence>
	inline Score search_next(int current_depth, int remaining_depth, Score alpha, Score beta)
	{
		constexpr Color next_color = flip_color(color);
		if constexpr (quiescence)
		{
			return search<next_color, true>(current_depth, 0, alpha, beta);
		}
		if (remaining_depth <= 0)
		{
			return search<next_color, true>(current_depth, 0, alpha, beta);
		}
		else
		{
			return search<next_color, false>(current_depth, remaining_depth, alpha, beta);
		}
	}

private:
	inline void reset()
	{
		_depth_limit = MAX_DEPTH;
		_stop_time = TIME_MAX;
		_node_limit = INT64_MAX;
		_node_count = 0;
		_start_time = get_time();
		_think_time = 0;
	}

	inline bool can_continue()
	{
		if (_depth == 0)
		{
			return true;
		}
		if (_node_count >= _node_limit)
		{
			return false;
		}
		if ((_node_count & 0x3FF) == 0 && get_time() >= _stop_time)
		{
			_node_limit = 0;
			return false;
		}
		return true;
	}

	inline void print_info()
	{
		std::cout << "info depth " << _depth;
		std::cout << " score ";
		if (_score >= SCORE_MATE - MAX_DEPTH)
		{
			std::cout << "mate " << (SCORE_MATE - _score + 1) / 2;
		}
		else if (_score <= -SCORE_MATE + MAX_DEPTH)
		{
			std::cout << "mate " << (-SCORE_MATE - _score - 1) / 2; 
		}
		else
		{
			std::cout << "cp " << _score;
		}
		Time elapsed_time = get_time() - _start_time;
		std::cout << " time " << elapsed_time;
		std::cout << " nodes " << _node_count;
		std::cout << " nps " << _node_count * 1000 / std::max(elapsed_time, 1LL);
		std::cout << " pv";
		int old_ply = _ply;
		while (true)
		{
			Hash hash = _hash_stack[_ply];
			TranspositionEntry entry = transposition_table.get_entry(hash);
			if (entry.hash != hash)
			{
				break;
			}
			Move entry_move = get_current_position().extract_move(entry.move);
			Move moves[MAX_MOVES];
			Move* end = generate_moves(moves);
			Move* move = std::find(moves, end, entry_move);
			if (move == end)
			{
				break;
			}
			if (!play_move(entry_move))
			{
				break;
			}
			std::cout << ' ' << entry_move;
			if (is_draw_by_repetition())
			{
				break;
			}
		}
		_ply = old_ply;
		std::cout << std::endl;
	}

	inline void print_move()
	{
		std::cout << "bestmove " << _move << std::endl;
	}

private:
	std::thread _thread;
	int _depth_limit;
	int64_t _node_limit;
	int64_t _node_count;
	Time _stop_time;
	Time _start_time;
	Time _think_time;
	int _depth;
	Score _score;
	Move _move;
};

inline Search search;
