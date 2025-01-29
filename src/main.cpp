#include "position.hpp"
#include "random.hpp"

#include <iostream>
#include <vector>

using namespace std;

template<Color color>
inline int64_t perft(const Position& position, int depth, Move* moves)
{
	constexpr Color enemy = flip_color(color);
	Move* end = position.generate_moves<color>(moves);
	if (depth == 1)
	{
		return end - moves;
	}
	int64_t result = 0;
	Position next_position;
	for (Move* move = moves; move != end; move++)
	{
		position.play_move<color>(&next_position, *move);
		result += perft<enemy>(next_position, depth - 1, end);
	}
	return result;
}

inline int64_t perft(const Position& position, int depth, Move* moves)
{
	switch (position.get_current_color())
	{
		case COLOR_WHITE:
			return perft<COLOR_WHITE>(position, depth, moves);
		case COLOR_BLACK:
			return perft<COLOR_BLACK>(position, depth, moves);
		default:
			return false;
	}
}

inline int64_t perft(const Position& position, int depth)
{
	Move* moves = new Move[100 * depth];
	int64_t result = perft(position, depth, moves);
	delete[] moves;
	return result;
}

int main()
{
	while (true) {


		string s = "3rkb1r/8/n3q3/8/8/4R2N/8/RQB1K3 w - - 0 1";

		Position p;
		p.set_fen(s);

		Move moves[999];
		Move* end = p.generate_moves<COLOR_WHITE>(moves);

		cout << end - moves << endl;

		int t=T();
		auto res =perft(p, 6);
		t=T()-t;

		cout << res << " "<<t<<" "<<res/(t+1) << endl;

		cout<<endl;
		cout<<endl;
		cout<<endl;
		cout<<endl;

	}

	return 0;
}
