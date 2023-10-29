module;

#include <sdl/SDL.h>

export module Time;

import Log;
import <vector>;

export class Time {
	Time() {}
	inline static Uint64 prevTime;
	inline static Uint64 currTime;

	inline static std::vector<double> deltaTimes;
	inline static size_t deltaTimesI = 0;

public:

	inline static Uint64 framecount = 0;
	inline static double deltaTime = 0.001, time = 0.0;
	inline static double smoothDeltaTime = 0.0;
	inline static float deltaTimeF;

	/// <summary> Updates time values, should be called once per frame </summary>
	static void Tick() {
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
		framecount++;
	}

	/// <summary> Returns the current accurate time, not tied to frame ticks </summary>
	static double GetCurrentRealtime() {
		return time + ((SDL_GetPerformanceCounter() - currTime) / (double)(SDL_GetPerformanceFrequency()));
	}
};