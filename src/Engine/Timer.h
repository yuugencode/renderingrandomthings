#pragma once

#include <vector>

// Simple class for timing things 
class Timer {
	size_t traceCnt = 0;
	double current = 0.0;
public:

	std::vector<double> times;

	Timer();
	Timer(size_t size);

	void Start();
	void End();
	double GetAveragedTime();
};
