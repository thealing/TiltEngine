#include "search.hpp"

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <ctime>
#include <chrono>
#include <thread>

using namespace std;

int main()
{
	Search search;
	string line;
	while (getline(cin, line))
	{
		stringstream ss(line);
		string word;
		ss >> word;
		if (word == "quit")
		{
			break;
		}
		if (word == "uci")
		{
			cout << "id name TiltEngine" << endl;
			cout << "id author Thealing" << endl;
			cout << "uciok" << endl;
			continue;
		}
		if (word == "isready")
		{
			cout << "readyok" << endl;
			continue;
		}
		if (word == "ucinewgame")
		{
			search.new_game();
			continue;
		}
		if (word == "position")
		{
			ss >> word;
			if (word == "fen")
			{
				string fen;
				while (ss >> word && word != "moves")
				{
					fen += word + ' ';
				}
				search.set_fen(fen.c_str());
			}
			else
			{
				search.set_fen(Game::START_FEN);
			}
			while (ss.good() && word != "moves")
			{
				ss >> word;
			}
			while (ss >> word)
			{
				Move move = search.parse_move(word.c_str());
				search.play_move(move);
			}
			continue;
		}
		if (word == "perft")
		{
			int depth;
			ss >> depth;
			Time start_time = get_time();
			int64_t result = search.perft(depth);
			Time end_time = get_time();
			Time elapsed_time = max(end_time - start_time, 1LL);
			int64_t nps = result * 1000 / elapsed_time;
			cout << "nodes : " << result << endl;
			cout << "time  : " << elapsed_time << endl;
			cout << "nps   : " << nps << endl;
			continue;
		}
		if (word == "go")
		{
			search.stop();
			int depth = 0;
			int64_t nodes = 0;
			Time move_time = 0;
			Time times[COLOR_COUNT] = {};
			Time increments[COLOR_COUNT] = {};
			int moves_to_go = 0;
			while (ss >> word)
			{
				if (word == "depth")
				{
					ss >> depth;
				}
				if (word == "nodes")
				{
					ss >> nodes;
				}
				if (word == "movetime")
				{
					ss >> move_time;
				}
				if (word == "wtime")
				{
					ss >> times[COLOR_WHITE];
				}
				if (word == "btime")
				{
					ss >> times[COLOR_BLACK];
				}
				if (word == "winc")
				{
					ss >> increments[COLOR_WHITE];
				}
				if (word == "binc")
				{
					ss >> increments[COLOR_BLACK];
				}
				if (word == "movestogo")
				{
					ss >> moves_to_go;
				}
			}
			if (times[COLOR_WHITE] != 0 || times[COLOR_BLACK] != 0)
			{
				search.start(times, increments, moves_to_go);
			}
			else
			{
				search.start(depth, nodes, move_time);
			}
			continue;
		}
		if (word == "stop")
		{
			search.stop();
			continue;
		}
		cout << "unknown command" << endl;
	}
	return 0;
}
