#pragma once

#include <vector>

// Simple class for timing things 
class Timer {
	size_t traceCnt;
	double current;
public:

	std::vector<double> times;

	Timer();
	Timer(size_t size);

	void Start();
	void End();
	double GetAveragedTime();
};
