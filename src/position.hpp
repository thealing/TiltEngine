#pragma once

#include "sliders.hpp"
#include "move.hpp"
#include "moves_helper.hpp"

#include <ctype.h>

struct Position
{
	static constexpr Bitboard CASTLING_MASK_WK = get_square_mask(SQUARE_E1, SQUARE_H1);
	static constexpr Bitboard CASTLING_MASK_WQ = get_square_mask(SQUARE_E1, SQUARE_A1);
	static constexpr Bitboard CASTLING_MASK_BK = get_square_mask(SQUARE_E8, SQUARE_H8);
	static constexpr Bitboard CASTLING_MASK_BQ = get_square_mask(SQUARE_E8, SQUARE_A8);

	Bitboard pieces[PIECE_COUNT];
	Bitboard colors[COLOR_COUNT];
	Bitboard castling_mask;
	Color current_color;
	Square en_passant_square;
	int8_t halfmove_clock;

	void set_fen(const char fen[])
	{
		memset(this, 0, sizeof(*this));
		for (Square square = 0; *fen != ' '; fen++)
		{
			if (isdigit(*fen))
			{
				square += Square(*fen - '0');
			}
			if (isalpha(*fen))
			{
				Piece piece = PIECE_NONE;
				switch (tolower(*fen))
				{
					case 'p':
						piece = PIECE_PAWN;
						break;
					case 'n':
						piece = PIECE_KNIGHT;
						break;
					case 'b':
						piece = PIECE_BISHOP;
						break;
					case 'r':
						piece = PIECE_ROOK;
						break;
					case 'q':
						piece = PIECE_QUEEN;
						break;
					case 'k':
						piece = PIECE_KING;
						break;
				}
				set_square(pieces[piece], square);
				if (isupper(*fen))
				{
					set_square(colors[COLOR_WHITE], square);
				}
				else
				{
					set_square(colors[COLOR_BLACK], square);
				}
				square++;
			}
		}
		fen++;
		switch (*fen)
		{
			case 'w':
				current_color = COLOR_WHITE;
				break;
			case 'b':
				current_color = COLOR_BLACK;
				break;
		}
		fen += 2;
		while (*fen != ' ')
		{
			switch (*fen)
			{
				case 'K': 
					castling_mask |= CASTLING_MASK_WK;
					break;
				case 'Q': 
					castling_mask |= CASTLING_MASK_WQ;
					break;
				case 'k': 
					castling_mask |= CASTLING_MASK_BK;
					break;
				case 'q': 
					castling_mask |= CASTLING_MASK_BQ;
					break;
			}
			fen++;
		}
		fen++;
		if (*fen == '-')
		{
			en_passant_square = SQUARE_NONE;
			fen += 2;
		}
		else
		{
			en_passant_square = parse_square(fen);
			fen += 3;
		}
		halfmove_clock = (int8_t)atoi(fen);
	}

	Square parse_square(const char str[]) const
	{
		return make_square('8' - str[1], str[0] - 'a');
	}

	Move parse_move(const char str[]) const
	{
		Move move;
		move.src_square = parse_square(str + 0);
		move.dst_square = parse_square(str + 2);
		move.type = get_piece(move.src_square);
		move.captured_piece = get_piece(move.dst_square);
		if (move.type == PIECE_PAWN)
		{
			if (abs(get_square_rank(move.dst_square) - get_square_rank(move.src_square)) == 2)
			{
				move.type = MOVE_TYPE_DOUBLE;
			}
			if (move.dst_square == en_passant_square)
			{
				move.type = MOVE_TYPE_EN_PASSANT;
			}
			switch (str[4])
			{
				case 'q':
					move.type = MOVE_TYPE_PROMOTION_Q;
					break;
				case 'r':
					move.type = MOVE_TYPE_PROMOTION_R;
					break;
				case 'b':
					move.type = MOVE_TYPE_PROMOTION_B;
					break;
				case 'n':
					move.type = MOVE_TYPE_PROMOTION_N;
					break;
			}
		}
		if (move.type == PIECE_KING)
		{
			if (move.src_square == SQUARE_E1 && move.dst_square == SQUARE_G1)
			{
				move.type = MOVE_TYPE_CASTLING_WK;
			}
			if (move.src_square == SQUARE_E1 && move.dst_square == SQUARE_C1)
			{
				move.type = MOVE_TYPE_CASTLING_WQ;
			}
			if (move.src_square == SQUARE_E8 && move.dst_square == SQUARE_G8)
			{
				move.type = MOVE_TYPE_CASTLING_BK;
			}
			if (move.src_square == SQUARE_E8 && move.dst_square == SQUARE_C8)
			{
				move.type = MOVE_TYPE_CASTLING_BQ;
			}
		}
		return move;
	}

	inline Move extract_move(uint16_t value) const
	{
		Square src_square = value & 0x3F;
		Square dst_square = (value >> 6) & 0x3F;
		MoveType type = MoveType(value >> 12);
		return Move{ src_square, dst_square, type, get_piece(dst_square) };
	}

	inline Piece get_piece(Square square) const
	{
		for (Piece piece = PIECE_PAWN; piece <= PIECE_KING; piece++)
		{
			if (test_square(pieces[piece], square))
			{
				return piece;
			}
		}
		return PIECE_NONE;
	}

	inline Bitboard get_mask(Piece piece, Color color) const
	{
		return pieces[piece] & colors[color];
	}

	inline Move* generate_moves(Move* move) const
	{
		switch (current_color)
		{
			case COLOR_WHITE:
				return generate_moves<COLOR_WHITE, false>(move);
			case COLOR_BLACK:
				return generate_moves<COLOR_BLACK, false>(move);
			default:
				return move;
		}
	}

	inline Move* generate_captures(Move* move) const
	{
		switch (current_color)
		{
			case COLOR_WHITE:
				return generate_moves<COLOR_WHITE, true>(move);
			case COLOR_BLACK:
				return generate_moves<COLOR_BLACK, true>(move);
			default:
				return move;
		}
	}

	inline void add_move(Move*& move, Square src_square, Square dst_square, MoveType type, Piece captured_piece) const
	{
		move->src_square = src_square;
		move->dst_square = dst_square;
		move->type = type;
		move->captured_piece = captured_piece;
		move++;
	}

	inline void add_move(Move*& move, Square src_square, Square dst_square, MoveType type) const
	{
		add_move(move, src_square, dst_square, type, PIECE_NONE);
	}

	inline void add_capture(Move*& move, Square src_square, Square dst_square, MoveType type) const
	{
		add_move(move, src_square, dst_square, type, get_piece(dst_square));
	}

	inline void add_pawn_move(Move*& move, Square src_square, Square dst_square) const
	{
		add_move(move, src_square, dst_square, PIECE_PAWN, PIECE_NONE);
	}

	inline void add_pawn_double_move(Move*& move, Square src_square, Square dst_square) const
	{
		add_move(move, src_square, dst_square, MOVE_TYPE_DOUBLE, PIECE_NONE);
	}

	inline void add_pawn_capture(Move*& move, Square src_square, Square dst_square) const
	{
		add_move(move, src_square, dst_square, PIECE_PAWN, get_piece(dst_square));
	}

	inline void add_pawn_promotion_moves(Move*& move, Square src_square, Square dst_square) const
	{
		for (MoveType type = MOVE_TYPE_PROMOTION_Q; type <= MOVE_TYPE_PROMOTION_N; type++)
		{
			move->src_square = src_square;
			move->dst_square = dst_square;
			move->type = type;
			move->captured_piece = PIECE_NONE;
			move++;
		}
	}

	inline void add_pawn_promotion_captures(Move*& move, Square src_square, Square dst_square) const
	{
		Piece captured_piece = get_piece(dst_square);
		for (MoveType type = MOVE_TYPE_PROMOTION_Q; type <= MOVE_TYPE_PROMOTION_N; type++)
		{
			move->src_square = src_square;
			move->dst_square = dst_square;
			move->type = type;
			move->captured_piece = captured_piece;
			move++;
		}
	}

	template<Color color, bool captures_only>
	inline Move* generate_moves(Move* move) const
	{
		constexpr Color enemy = flip_color(color);
		constexpr Rank starting_rank = get_starting_rank<color>();
		constexpr Bitboard starting_rank_mask = get_rank_mask(starting_rank);
		constexpr Rank promotion_rank = get_promotion_rank<color>();
		constexpr Bitboard promotion_rank_mask = get_rank_mask(promotion_rank);
		Bitboard color_mask = colors[color];
		Bitboard enemy_mask = colors[enemy];
		Bitboard occupied_mask = color_mask | enemy_mask;
		Bitboard empty_mask = ~occupied_mask;
		Bitboard pawn_mask = color_mask & pieces[PIECE_PAWN];
		Bitboard pawn_move_mask = empty_mask & shift_forward<color>(pawn_mask);
		Bitboard pawn_left_capture_mask = enemy_mask & shift_forward_left<color>(pawn_mask);
		Bitboard pawn_right_capture_mask = enemy_mask & shift_forward_right<color>(pawn_mask);
		Bitboard src_mask;
		Bitboard dst_mask;
		if constexpr (!captures_only)
		{
			dst_mask = pawn_move_mask & ~promotion_rank_mask;
			while (dst_mask != 0)
			{
				Square dst_square = pop_square(dst_mask);
				Square src_square = move_backward<color>(dst_square);
				add_pawn_move(move, src_square, dst_square);
			}
			dst_mask = pawn_mask & starting_rank_mask;
			dst_mask = shift_forward<color>(dst_mask);
			dst_mask &= empty_mask;
			dst_mask = shift_forward<color>(dst_mask);
			dst_mask &= empty_mask;
			while (dst_mask != 0)
			{
				Square dst_square = pop_square(dst_mask);
				Square src_square = move_backward<color, 2>(dst_square);
				add_pawn_double_move(move, src_square, dst_square);
			}
		}
		dst_mask = pawn_move_mask & promotion_rank_mask;
		while (dst_mask != 0)
		{
			Square dst_square = pop_square(dst_mask);
			Square src_square = move_backward<color>(dst_square);
			add_pawn_promotion_moves(move, src_square, dst_square);
		}
		dst_mask = pawn_left_capture_mask & ~promotion_rank_mask;
		while (dst_mask != 0)
		{
			Square dst_square = pop_square(dst_mask);
			Square src_square = move_backward_right<color>(dst_square);
			add_pawn_capture(move, src_square, dst_square);
		}
		dst_mask = pawn_right_capture_mask & ~promotion_rank_mask;
		while (dst_mask != 0)
		{
			Square dst_square = pop_square(dst_mask);
			Square src_square = move_backward_left<color>(dst_square);
			add_pawn_capture(move, src_square, dst_square);
		}
		dst_mask = pawn_left_capture_mask & promotion_rank_mask;
		while (dst_mask != 0)
		{
			Square dst_square = pop_square(dst_mask);
			Square src_square = move_backward_right<color>(dst_square);
			add_pawn_promotion_captures(move, src_square, dst_square);
		}
		dst_mask = pawn_right_capture_mask & promotion_rank_mask;
		while (dst_mask != 0)
		{
			Square dst_square = pop_square(dst_mask);
			Square src_square = move_backward_left<color>(dst_square);
			add_pawn_promotion_captures(move, src_square, dst_square);
		}
		if (en_passant_square != SQUARE_NONE)
		{
			if (get_square_file(en_passant_square) != FILE_A)
			{
				Square src_square = move_backward_left<color>(en_passant_square);
				if (test_square(pawn_mask, src_square))
				{
					move->src_square = src_square;
					move->dst_square = en_passant_square;
					move->type = MOVE_TYPE_EN_PASSANT;
					move->captured_piece = PIECE_NONE;
					move++;
				}
			}
			if (get_square_file(en_passant_square) != FILE_H)
			{
				Square src_square = move_backward_right<color>(en_passant_square);
				if (test_square(pawn_mask, src_square))
				{
					move->src_square = src_square;
					move->dst_square = en_passant_square;
					move->type = MOVE_TYPE_EN_PASSANT;
					move->captured_piece = PIECE_NONE;
					move++;
				}
			}
		}
		for (Piece piece = PIECE_KNIGHT; piece <= PIECE_KING; piece++)
		{
			src_mask = color_mask & pieces[piece];
			while (src_mask != 0)
			{
				Square src_square = pop_square(src_mask);
				Bitboard move_mask = 0;
				switch (piece)
				{
					case PIECE_KNIGHT:
						move_mask = bitmasks.get_knight_mask(src_square);
						break;
					case PIECE_BISHOP:
						move_mask = sliders.get_bishop_mask(src_square, occupied_mask);
						break;
					case PIECE_ROOK:
						move_mask = sliders.get_rook_mask(src_square, occupied_mask);
						break;
					case PIECE_QUEEN:
						move_mask = sliders.get_queen_mask(src_square, occupied_mask);
						break;
					case PIECE_KING:
						move_mask = bitmasks.get_king_mask(src_square);
						break;
				}
				if constexpr (!captures_only)
				{
					dst_mask = move_mask & empty_mask;
					while (dst_mask != 0)
					{
						Square dst_square = pop_square(dst_mask);
						add_move(move, src_square, dst_square, piece);
					}
				}
				dst_mask = move_mask & enemy_mask;
				while (dst_mask != 0)
				{
					Square dst_square = pop_square(dst_mask);
					add_capture(move, src_square, dst_square, piece);
				}
			}
		}
		if ((castling_mask & color_mask & pieces[PIECE_KING]) != 0 && !is_in_check<color>())
		{
			if constexpr (color == COLOR_WHITE)
			{
				if ((castling_mask & CASTLING_MASK_WK) == CASTLING_MASK_WK && test_square(empty_mask, SQUARE_F1) && test_square(empty_mask, SQUARE_G1) && !is_attacked<COLOR_BLACK>(SQUARE_F1))
				{
					move->src_square = SQUARE_E1;
					move->dst_square = SQUARE_G1;
					move->type = MOVE_TYPE_CASTLING_WK;
					move->captured_piece = PIECE_NONE;
					move++;
				}
				if ((castling_mask & CASTLING_MASK_WQ) == CASTLING_MASK_WQ && test_square(empty_mask, SQUARE_D1) && test_square(empty_mask, SQUARE_C1) && test_square(empty_mask, SQUARE_B1) && !is_attacked<COLOR_BLACK>(SQUARE_D1))
				{
					move->src_square = SQUARE_E1;
					move->dst_square = SQUARE_C1;
					move->type = MOVE_TYPE_CASTLING_WQ;
					move->captured_piece = PIECE_NONE;
					move++;
				}
			}
			if constexpr (color == COLOR_BLACK)
			{
				if ((castling_mask & CASTLING_MASK_BK) == CASTLING_MASK_BK && test_square(empty_mask, SQUARE_F8) && test_square(empty_mask, SQUARE_G8) && !is_attacked<COLOR_WHITE>(SQUARE_F8))
				{
					move->src_square = SQUARE_E8;
					move->dst_square = SQUARE_G8;
					move->type = MOVE_TYPE_CASTLING_BK;
					move->captured_piece = PIECE_NONE;
					move++;
				}
				if ((castling_mask & CASTLING_MASK_BQ) == CASTLING_MASK_BQ && test_square(empty_mask, SQUARE_D8) && test_square(empty_mask, SQUARE_C8) && test_square(empty_mask, SQUARE_B8) && !is_attacked<COLOR_WHITE>(SQUARE_D8))
				{
					move->src_square = SQUARE_E8;
					move->dst_square = SQUARE_C8;
					move->type = MOVE_TYPE_CASTLING_BQ;
					move->captured_piece = PIECE_NONE;
					move++;
				}
			}
		}
		return move;
	}

	inline bool is_in_check() const
	{
		switch (current_color)
		{
			case COLOR_WHITE:
				return is_in_check<COLOR_WHITE>();
			case COLOR_BLACK:
				return is_in_check<COLOR_BLACK>();
			default:
				return false;
		}
	}

	template<Color color>
	inline bool is_in_check() const
	{
		constexpr Color enemy = flip_color(color);
		const Square king_square = get_square(colors[color] & pieces[PIECE_KING]);
		return is_attacked<enemy>(king_square);
	}

	template<Color color>
	inline bool is_attacked(Square square) const
	{
		constexpr Color enemy = flip_color(color);
		Bitboard color_mask = colors[color];
		Bitboard occupied_mask = color_mask | colors[enemy];
		Bitboard src_mask;
		src_mask = color_mask & (pieces[PIECE_BISHOP] | pieces[PIECE_QUEEN]);
		if (src_mask != 0 && (src_mask & bitmasks.get_bishop_mask(square)) != 0 && (src_mask & sliders.get_bishop_mask(square, occupied_mask)) != 0)
		{
			return true;
		}
		src_mask = color_mask & (pieces[PIECE_ROOK] | pieces[PIECE_QUEEN]);
		if (src_mask != 0 && (src_mask & bitmasks.get_rook_mask(square)) != 0 && (src_mask & sliders.get_rook_mask(square, occupied_mask)) != 0)
		{
			return true;
		}
		src_mask = color_mask & pieces[PIECE_KNIGHT];
		if (src_mask != 0 && (src_mask & bitmasks.get_knight_mask(square)) != 0)
		{
			return true;
		}
		src_mask = color_mask & pieces[PIECE_KING];
		if (src_mask != 0 && (src_mask & bitmasks.get_king_mask(square)) != 0)
		{
			return true;
		}
		Bitboard pawn_mask = color_mask & pieces[PIECE_PAWN];
		Bitboard pawn_left_capture_mask = shift_forward_left<color>(pawn_mask);
		Bitboard pawn_right_capture_mask = shift_forward_right<color>(pawn_mask);
		if ((get_square_mask(square) & (pawn_left_capture_mask | pawn_right_capture_mask)) != 0)
		{
			return true;
		}
		return false;
	}

	inline bool play_move(Position* next_position, const Move& move) const
	{
		switch (current_color)
		{
			case COLOR_WHITE:
				return play_move<COLOR_WHITE>(next_position, move);
			case COLOR_BLACK:
				return play_move<COLOR_BLACK>(next_position, move);
			default:
				return false;
		}
	}

	template<Color color>
	inline bool play_move(Position* next_position, const Move& move) const
	{
		constexpr Color enemy = flip_color(color);
		Bitboard src_mask = get_square_mask(move.src_square);
		Bitboard dst_mask = get_square_mask(move.dst_square);
		Bitboard move_mask = src_mask | dst_mask;
		Bitboard en_passant_mask = shift_backward<color>(dst_mask);
		memcpy(next_position, this, sizeof(pieces) + sizeof(colors));
		next_position->current_color = enemy;
		next_position->castling_mask = castling_mask & ~move_mask;
		next_position->en_passant_square = SQUARE_NONE;
		next_position->colors[color] ^= move_mask;
		if (move.captured_piece != PIECE_NONE)
		{
			next_position->pieces[move.captured_piece] ^= dst_mask;
			next_position->colors[enemy] ^= dst_mask;
			next_position->halfmove_clock = 0;
		}
		else
		{
			next_position->halfmove_clock = halfmove_clock + 1;
		}
		switch (move.type)
		{
			case MOVE_TYPE_DOUBLE:
				next_position->pieces[PIECE_PAWN] ^= move_mask;
				next_position->en_passant_square = move_backward<color>(move.dst_square);
				next_position->halfmove_clock = 0;
				break;
			case MOVE_TYPE_EN_PASSANT:
				next_position->pieces[PIECE_PAWN] ^= move_mask | en_passant_mask;
				next_position->colors[enemy] ^= en_passant_mask;
				next_position->halfmove_clock = 0;
				break;
			case MOVE_TYPE_PROMOTION_Q:
				next_position->pieces[PIECE_PAWN] ^= src_mask;
				next_position->pieces[PIECE_QUEEN] ^= dst_mask;
				next_position->halfmove_clock = 0;
				break;
			case MOVE_TYPE_PROMOTION_R:
				next_position->pieces[PIECE_PAWN] ^= src_mask;
				next_position->pieces[PIECE_ROOK] ^= dst_mask;
				next_position->halfmove_clock = 0;
				break;
			case MOVE_TYPE_PROMOTION_B:
				next_position->pieces[PIECE_PAWN] ^= src_mask;
				next_position->pieces[PIECE_BISHOP] ^= dst_mask;
				next_position->halfmove_clock = 0;
				break;
			case MOVE_TYPE_PROMOTION_N:
				next_position->pieces[PIECE_PAWN] ^= src_mask;
				next_position->pieces[PIECE_KNIGHT] ^= dst_mask;
				next_position->halfmove_clock = 0;
				break;
			case MOVE_TYPE_CASTLING_WK:
				next_position->pieces[PIECE_KING] ^= get_square_mask(SQUARE_E1, SQUARE_G1);
				next_position->pieces[PIECE_ROOK] ^= get_square_mask(SQUARE_H1, SQUARE_F1);
				next_position->colors[color] ^= get_square_mask(SQUARE_H1, SQUARE_F1);
				break;
			case MOVE_TYPE_CASTLING_WQ:
				next_position->pieces[PIECE_KING] ^= get_square_mask(SQUARE_E1, SQUARE_C1);
				next_position->pieces[PIECE_ROOK] ^= get_square_mask(SQUARE_A1, SQUARE_D1);
				next_position->colors[color] ^= get_square_mask(SQUARE_A1, SQUARE_D1);
				break;
			case MOVE_TYPE_CASTLING_BK:
				next_position->pieces[PIECE_KING] ^= get_square_mask(SQUARE_E8, SQUARE_G8);
				next_position->pieces[PIECE_ROOK] ^= get_square_mask(SQUARE_H8, SQUARE_F8);
				next_position->colors[color] ^= get_square_mask(SQUARE_H8, SQUARE_F8);
				break;
			case MOVE_TYPE_CASTLING_BQ:
				next_position->pieces[PIECE_KING] ^= get_square_mask(SQUARE_E8, SQUARE_C8);
				next_position->pieces[PIECE_ROOK] ^= get_square_mask(SQUARE_A8, SQUARE_D8);
				next_position->colors[color] ^= get_square_mask(SQUARE_A8, SQUARE_D8);
				break;
			default:
				next_position->pieces[move.type] ^= move_mask;
				break;
		}
		return !next_position->is_in_check<color>();
	}
};
