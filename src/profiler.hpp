#pragma once

/*
	- Profiler for measuring exact time percentage spent in blocks (in optimized build).
	- Runs in a separate thread which slows down the program by ~10%.
	- Starts lazily on the first use.
*/

#include <iostream>
#include <thread>

class Stopwatch
{
public:
	Stopwatch()
	{
		_start_time = get_time();
	}

	inline double get_elapsed_time() const
	{
		return get_time() - _start_time;
	}

private:
	static inline double get_time()
	{
		using namespace std::chrono;
		duration<double> time = system_clock::now().time_since_epoch();
		return time.count();
	}

private:
	double _start_time;
};

class Profiler
{
	static constexpr double REPORT_INTERVAL = 1.0;

public:
	static Profiler& get()
	{
		static Profiler instance;
		return instance;
	}

public:
	void enter()
	{
		_active = true;
	}

	void leave()
	{
		_active = false;
	}

private:
	Profiler() : _active(false), _thread(&Profiler::watch, this)
	{
	}

	~Profiler()
	{
		_thread.detach();
	}

	void watch()
	{
		std::cerr << "\033[31m";
		std::cerr << "Profiler started!" << std::endl;
		std::cerr << "\033[0m";
		while (true)
		{
			int total_count = 0;
			int active_count = 0;
			Stopwatch stopwatch;
			while (stopwatch.get_elapsed_time() < REPORT_INTERVAL)
			{
				wait(0.0011);
				if (_active)
				{
					active_count++;
				}
				total_count++;
			}
			std::cerr << "\033[31m";
			std::cerr << "Usage: " << std::lround(100.0 * active_count / total_count) << '%' << std::endl;
			std::cerr << "\033[0m";
		}
	}

	void wait(double duration)
	{
		Stopwatch stopwatch;
		while (stopwatch.get_elapsed_time() < duration)
		{
		}
	}

private:
	bool _active;
	std::thread _thread;
};
