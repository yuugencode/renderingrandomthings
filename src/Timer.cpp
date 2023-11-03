#include "Timer.h"

#include "Time.h"

Timer::Timer() : current(0.0), traceCnt(0) { times.reserve(32); }
Timer::Timer(size_t size) : current(0.0), traceCnt(0) {
	times.reserve(size);
}

void Timer::Start() {
	current = Time::GetAccurateTime();
}

void Timer::End() {
	double delta = Time::GetAccurateTime() - current;
	if (times.size() < times.capacity()) times.push_back(delta);
	else times[(traceCnt++) % times.size()] = delta;
}

double Timer::GetAveragedTime() {
	double sum = 0.0;
	for (size_t i = 0; i < times.size(); i++)
		sum += times[i];
	return sum / times.size();
}
