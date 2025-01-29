#pragma once

#include "move.hpp"
#include "sliders.hpp"

#include <sstream>
#include <charconv>

class Position
{
public:
	void set_fen(std::string_view s)
	{
		clear();
		std::stringstream fen;
		fen << s;
		std::string field;
		fen >> field;
		set_board(field);
		fen >> field;
		set_current_color(field);
		fen >> field;
		set_castling_mask(field);
		fen >> field;
		set_en_passant_square(field);
		fen >> field;
		set_halfmove_clock(field);
	}

	Square parse_square(std::string_view s) const
	{
		return make_square('8' - s[1], s[0] - 'a');
	}

	template<Color color>
	Move* generate_moves(Move* moves) const
	{
		constexpr Color enemy = flip_color(color);
		Bitboard color_mask = _colors[color];
		Bitboard enemy_mask = _colors[enemy];
		Bitboard occupied_mask = color_mask | enemy_mask;
		Bitboard empty_mask = ~occupied_mask;
		Bitboard king_mask = _pieces[PIECE_KING] & color_mask;
		Square king_square = get_square(king_mask);
		Bitboard king_knight_mask = Bitboards::get_knight_mask(king_square);
		Bitboard king_bishop_mask = Sliders::get_bishop_mask(king_square, occupied_mask);
		Bitboard king_rook_mask = Sliders::get_rook_mask(king_square, occupied_mask);
		Bitboard enemy_knight_mask = enemy_mask & _pieces[PIECE_KNIGHT];
		Bitboard enemy_bishop_mask = enemy_mask & (_pieces[PIECE_BISHOP] | _pieces[PIECE_QUEEN]);
		Bitboard enemy_rook_mask = enemy_mask & (_pieces[PIECE_ROOK] | _pieces[PIECE_QUEEN]);
		Bitboard slider_checker_mask = 0;
		slider_checker_mask |= king_bishop_mask & enemy_bishop_mask;
		slider_checker_mask |= king_rook_mask & enemy_rook_mask;
		Bitboard simple_checker_mask = 0;
		simple_checker_mask |= king_knight_mask & enemy_knight_mask;
		Bitboard checker_mask = slider_checker_mask | simple_checker_mask;
		Bitboard check_mask;
		if (checker_mask == 0) {
			check_mask = BITBOARD_FULL;
		}
		else if (has_multiple_squares(checker_mask)) {
			check_mask = BITBOARD_EMPTY;
		}
		else {
			check_mask = Bitboards::get_check_mask(king_square, get_square(checker_mask));
		}
		Bitboard king_bishop_xray_mask = (king_bishop_mask ^ Sliders::get_bishop_mask(king_square, occupied_mask & ~king_bishop_mask)) & enemy_bishop_mask;
		Bitboard king_rook_xray_mask = (king_rook_mask ^ Sliders::get_rook_mask(king_square, occupied_mask & ~king_rook_mask)) & enemy_rook_mask;
		Bitboard bishop_pin_mask = 0;
		Bitboard rook_pin_mask = 0;
		while (king_bishop_xray_mask != 0) {
			Square pinner_square = pop_square(king_bishop_xray_mask);
			bishop_pin_mask |= Bitboards::get_check_mask(king_square, pinner_square);
		}
		while (king_rook_xray_mask != 0) {
			Square pinner_square = pop_square(king_rook_xray_mask);
			rook_pin_mask |= Bitboards::get_check_mask(king_square, pinner_square);
		}
		{
			Piece piece = PIECE_KNIGHT;
			Bitboard src_mask = color_mask & _pieces[piece] & ~(rook_pin_mask | bishop_pin_mask);
			while (src_mask != 0) {
				Square src_square = pop_square(src_mask);
				Bitboard move_mask = Bitboards::get_knight_mask(src_square);
				move_mask &= check_mask;
				Bitboard dst_mask;
				dst_mask = move_mask & empty_mask;
				while (dst_mask != 0) {
					Square dst_square = pop_square(dst_mask);
					add_move(moves, src_square, dst_square, piece);
				}
				dst_mask = move_mask & enemy_mask;
				while (dst_mask != 0) {
					Square dst_square = pop_square(dst_mask);
					add_capture(moves, src_square, dst_square, piece);
				}
			}
		}
		{
			Piece piece = PIECE_BISHOP;
			Bitboard src_mask = color_mask & _pieces[piece] & ~rook_pin_mask;
			while (src_mask != 0) {
				Square src_square = pop_square(src_mask);

				Bitboard move_mask = Sliders::get_bishop_mask(src_square, occupied_mask);
				move_mask &= check_mask;

				if (get_square_mask(src_square) & bishop_pin_mask)
				{
					move_mask &= bishop_pin_mask;
				}
				Bitboard dst_mask;
				dst_mask = move_mask & empty_mask;
				while (dst_mask != 0) {
					Square dst_square = pop_square(dst_mask);
					add_move(moves, src_square, dst_square, piece);
				}
				dst_mask = move_mask & enemy_mask;
				while (dst_mask != 0) {
					Square dst_square = pop_square(dst_mask);
					add_capture(moves, src_square, dst_square, piece);
				}
			}
		}
		{
			Piece piece = PIECE_ROOK;
			Bitboard src_mask = color_mask & _pieces[piece] & ~bishop_pin_mask;
			while (src_mask != 0) {
				Square src_square = pop_square(src_mask);

				Bitboard move_mask = Sliders::get_rook_mask(src_square, occupied_mask);
				move_mask &= check_mask;

				if (get_square_mask(src_square) & rook_pin_mask)
				{
					move_mask &= rook_pin_mask;
				}
				Bitboard dst_mask;
				dst_mask = move_mask & empty_mask;
				while (dst_mask != 0) {
					Square dst_square = pop_square(dst_mask);
					add_move(moves, src_square, dst_square, piece);
				}
				dst_mask = move_mask & enemy_mask;
				while (dst_mask != 0) {
					Square dst_square = pop_square(dst_mask);
					add_capture(moves, src_square, dst_square, piece);
				}
			}
		}
		{
			Piece piece = PIECE_QUEEN;
			Bitboard src_mask = color_mask & _pieces[piece];
			while (src_mask != 0) {
				Square src_square = pop_square(src_mask);
				Bitboard move_mask;
				if ((get_square_mask(src_square) & rook_pin_mask) == 0){
					move_mask = Sliders::get_bishop_mask(src_square, occupied_mask);
					move_mask &= check_mask;

					if (get_square_mask(src_square) & bishop_pin_mask)
					{
						move_mask &= bishop_pin_mask;
					}

					Bitboard dst_mask;
					dst_mask = move_mask & empty_mask;
					while (dst_mask != 0) {
						Square dst_square = pop_square(dst_mask);
						add_move(moves, src_square, dst_square, piece);
					}
					dst_mask = move_mask & enemy_mask;
					while (dst_mask != 0) {
						Square dst_square = pop_square(dst_mask);
						add_capture(moves, src_square, dst_square, piece);
					}
				}
				if((get_square_mask(src_square) & bishop_pin_mask) == 0){
					move_mask = Sliders::get_rook_mask(src_square, occupied_mask);
					move_mask &= check_mask;

					if (get_square_mask(src_square) & rook_pin_mask)
					{
						move_mask &= rook_pin_mask;
					}
					Bitboard dst_mask;
					dst_mask = move_mask & empty_mask;
					while (dst_mask != 0) {
						Square dst_square = pop_square(dst_mask);
						add_move(moves, src_square, dst_square, piece);
					}
					dst_mask = move_mask & enemy_mask;
					while (dst_mask != 0) {
						Square dst_square = pop_square(dst_mask);
						add_capture(moves, src_square, dst_square, piece);
					}
				}
			}
			//PE();
			{
				Piece piece = PIECE_KING;
				Bitboard src_mask = color_mask & _pieces[piece];
				while (src_mask != 0) {
					Square src_square = pop_square(src_mask);
					Bitboard move_mask = Bitboards::get_king_mask(src_square);
					Bitboard king_removed_mask = occupied_mask & ~king_mask;
					Bitboard attacker_mask = enemy_mask;
					while (attacker_mask)
					{
						Square square = pop_square(attacker_mask);
						//PE();
						Piece piece = get_piece(square); // SLOW!!!
						//PL();
						move_mask &= ~generate_moves(square, piece, king_removed_mask);
					}
					Bitboard dst_mask;
					dst_mask = move_mask & empty_mask;
					while (dst_mask != 0) {
						Square dst_square = pop_square(dst_mask);
						add_move(moves, src_square, dst_square, piece);
					}
					dst_mask = move_mask & enemy_mask;
					while (dst_mask != 0) {
						Square dst_square = pop_square(dst_mask);
						add_capture(moves, src_square, dst_square, piece);
					}
				}
			}
			//PL();
		}
		return moves;
	}

	inline Piece get_piece(Square square) const
	{
		for (Piece piece = 0; piece < PIECE_COUNT; piece++) {
			if (test_square(_pieces[piece], square)) {
				return piece;
			}
		}
		return PIECE_NONE;
	}

	// TEMP
	inline Bitboard generate_moves(Square square, Piece piece, Bitboard occupied_mask) const
	{
		switch (piece)
		{
			case PIECE_KNIGHT:
			{
				return Bitboards::get_knight_mask(square);
			}
			case PIECE_BISHOP:
			{
				return Sliders::get_bishop_mask(square, occupied_mask);
			}
			case PIECE_ROOK:
			{
				return Sliders::get_rook_mask(square, occupied_mask);
			}
			case PIECE_QUEEN:
			{
				return Sliders::get_queen_mask(square, occupied_mask);
			}
			case PIECE_KING:
			{
				return Bitboards::get_king_mask(square);
			}
			default:
			{
				std::unreachable();
			}
		}
	}

	template<Color color>
	inline void play_move(Position* next_position, Move move) const
	{
		constexpr Color enemy = flip_color(color);
		Bitboard src_mask = get_square_mask(move.src_square);
		Bitboard dst_mask = get_square_mask(move.dst_square);
		Bitboard move_mask = src_mask | dst_mask;
		Bitboard en_passant_mask = shift_backward<color>(dst_mask);
		memcpy(next_position, this, sizeof(_colors) + sizeof(_pieces));
		next_position->_current_color = enemy;
		next_position->_castling_mask = _castling_mask & ~move_mask;
		next_position->_en_passant_square = SQUARE_NONE;
		next_position->_colors[color] ^= move_mask;
		if (move.captured_piece != PIECE_NONE) {
			next_position->_pieces[move.captured_piece] &= ~dst_mask;
			next_position->_colors[enemy] &= ~dst_mask;
			next_position->_halfmove_clock = 0;
		}
		else {
			next_position->_halfmove_clock = _halfmove_clock + 1;
		}
		switch (move.type) {
				next_position->_pieces[PIECE_PAWN] ^= move_mask;
				next_position->_en_passant_square = move_backward<color>(move.dst_square);
				next_position->_halfmove_clock = 0;
				break;
			case MOVE_TYPE_EN_PASSANT:
				next_position->_pieces[PIECE_PAWN] ^= move_mask | en_passant_mask;
				next_position->_colors[enemy] ^= en_passant_mask;
				next_position->_halfmove_clock = 0;
				break;
			case MOVE_TYPE_PROMOTION_Q:
				next_position->_pieces[PIECE_PAWN] ^= src_mask;
				next_position->_pieces[PIECE_QUEEN] ^= dst_mask;
				next_position->_halfmove_clock = 0;
				break;
			case MOVE_TYPE_PROMOTION_R:
				next_position->_pieces[PIECE_PAWN] ^= src_mask;
				next_position->_pieces[PIECE_ROOK] ^= dst_mask;
				next_position->_halfmove_clock = 0;
				break;
			case MOVE_TYPE_PROMOTION_B:
				next_position->_pieces[PIECE_PAWN] ^= src_mask;
				next_position->_pieces[PIECE_BISHOP] ^= dst_mask;
				next_position->_halfmove_clock = 0;
				break;
			case MOVE_TYPE_PROMOTION_N:
				next_position->_pieces[PIECE_PAWN] ^= src_mask;
				next_position->_pieces[PIECE_KNIGHT] ^= dst_mask;
				next_position->_halfmove_clock = 0;
				break;
			case MOVE_TYPE_CASTLING_WK:
				next_position->_pieces[PIECE_KING] ^= get_square_mask(SQUARE_E1, SQUARE_G1);
				next_position->_pieces[PIECE_ROOK] ^= get_square_mask(SQUARE_H1, SQUARE_F1);
				next_position->_colors[color] ^= get_square_mask(SQUARE_H1, SQUARE_F1);
				break;
			case MOVE_TYPE_CASTLING_WQ:
				next_position->_pieces[PIECE_KING] ^= get_square_mask(SQUARE_E1, SQUARE_C1);
				next_position->_pieces[PIECE_ROOK] ^= get_square_mask(SQUARE_A1, SQUARE_D1);
				next_position->_colors[color] ^= get_square_mask(SQUARE_A1, SQUARE_D1);
				break;
			case MOVE_TYPE_CASTLING_BK:
				next_position->_pieces[PIECE_KING] ^= get_square_mask(SQUARE_E8, SQUARE_G8);
				next_position->_pieces[PIECE_ROOK] ^= get_square_mask(SQUARE_H8, SQUARE_F8);
				next_position->_colors[color] ^= get_square_mask(SQUARE_H8, SQUARE_F8);
				break;
			case MOVE_TYPE_CASTLING_BQ:
				next_position->_pieces[PIECE_KING] ^= get_square_mask(SQUARE_E8, SQUARE_C8);
				next_position->_pieces[PIECE_ROOK] ^= get_square_mask(SQUARE_A8, SQUARE_D8);
				next_position->_colors[color] ^= get_square_mask(SQUARE_A8, SQUARE_D8);
				break;
			default:
				next_position->_pieces[move.type] ^= move_mask;
				break;
		}
	}

	inline Color get_current_color() const
	{
		return _current_color;
	}

private:
	inline void clear() 
	{
		memset(this, 0, sizeof(*this));
	}

	inline void set_board(std::string_view s) 
	{
		Square square = 0;
		for (char c : s) {
			if (isdigit(c)) {
				square += Square(c - '0');
			}
			if (isalpha(c)) {
				Piece piece = PIECE_NONE;
				switch (tolower(c)) {
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
					default:
						std::unreachable();
				}
				set_square(_pieces[piece], square);
				if (isupper(c)) {
					set_square(_colors[COLOR_WHITE], square);
				}
				else {
					set_square(_colors[COLOR_BLACK], square);
				}
				square++;
			}
		}
	}

	inline void set_current_color(std::string_view s) 
	{
		if (s == "w") {
			_current_color = COLOR_WHITE;
		}
		if (s == "b") {
			_current_color = COLOR_BLACK;
		}
	}

	inline void set_castling_mask(std::string_view s) 
	{
		for (char c : s) {
			switch (c) {
				case 'K':
					_castling_mask |= CASTLING_MASK_WK;
					break;
				case 'Q':
					_castling_mask |= CASTLING_MASK_WQ;
					break;
				case 'k':
					_castling_mask |= CASTLING_MASK_BK;
					break;
				case 'q':
					_castling_mask |= CASTLING_MASK_BQ;
					break;
			}
		}
	}

	inline void set_en_passant_square(std::string_view s) 
	{
		if (s == "-") {
			_en_passant_square = SQUARE_NONE;
		}
		else {
			_en_passant_square = parse_square(s);
		}
	}

	inline void set_halfmove_clock(std::string_view s) 
	{
		std::from_chars(s.data(), s.data() + s.size(), _halfmove_clock);
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

private:
	static constexpr Bitboard CASTLING_MASK_WK = get_square_mask(SQUARE_E1, SQUARE_H1);
	static constexpr Bitboard CASTLING_MASK_WQ = get_square_mask(SQUARE_E1, SQUARE_A1);
	static constexpr Bitboard CASTLING_MASK_BK = get_square_mask(SQUARE_E8, SQUARE_H8);
	static constexpr Bitboard CASTLING_MASK_BQ = get_square_mask(SQUARE_E8, SQUARE_A8);

private:
	Bitboard _colors[COLOR_COUNT];
	Bitboard _pieces[PIECE_COUNT];
	Bitboard _castling_mask;
	Color _current_color;
	Square _en_passant_square;
	int8_t _halfmove_clock;
};
