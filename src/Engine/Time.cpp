#include "Time.h"

Uint64 Time::prevTime = 0;
Uint64 Time::currTime = 0;
size_t Time::deltaTimesI = 0;
Uint64 Time::framecount = 0;
double Time::deltaTime = 0.001, Time::time = 0.0;
double Time::smoothDeltaTime = 0.0;
float Time::deltaTimeF = 0.0f, Time::timeF = 0.0f;
std::vector<double> Time::deltaTimes;

void Time::Tick() {
	prevTime = currTime;
	currTime = SDL_GetPerformanceCounter();
	deltaTime = ((currTime - prevTime) / (double)(SDL_GetPerformanceFrequency()));
	if (framecount == 0) deltaTime = 0.001; // Fake deltatime for the first frame
	deltaTimeF = (float)deltaTime;

	if (deltaTimes.size() < 64) deltaTimes.push_back(deltaTime);
	else deltaTimes[(deltaTimesI++) % deltaTimes.size()] = deltaTime;

	smoothDeltaTime = 0.0;
	for (size_t i = 0; i < deltaTimes.size(); i++)
		smoothDeltaTime += deltaTimes[i];
	smoothDeltaTime /= deltaTimes.size();

	time += deltaTime;
	timeF = (float)time;
	framecount++;
}

double Time::GetAccurateTime() {
	return time + ((SDL_GetPerformanceCounter() - currTime) / (double)(SDL_GetPerformanceFrequency()));
}
