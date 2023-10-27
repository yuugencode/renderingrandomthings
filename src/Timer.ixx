export module Timer;

import Time;
import <vector>;

/// <summary> Simple class for timing things </summary>
export class Timer {
	size_t traceCnt;
	double current;
public:

	std::vector<double> times;

	Timer(size_t size) {
		times.reserve(size);
	}

	void Start() {
		current = Time::GetCurrentRealtime();
	}

	void End() {
		double delta = Time::GetCurrentRealtime() - current;
		if (times.size() < times.capacity()) times.push_back(delta);
		else times[(traceCnt++) % times.size()] = delta;
	}

	double GetAveragedTime() {
		double sum = 0.0;
		for (size_t i = 0; i < times.size(); i++)
			sum += times[i];
		return sum / times.size();
	}
};