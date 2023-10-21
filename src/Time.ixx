module;

#include <sdl/SDL.h>

export module Time;

import Log;

export class Time {
	Time() {}
	inline static Uint64 prevTime;
	inline static Uint64 currTime;

public:

	inline static Uint64 framecount = 0;
	inline static double deltaTime = 0.001, time = 0.0;

	/// <summary> Updates time values, should be called once per frame </summary>
	static void Tick() {
		prevTime = currTime;
		currTime = SDL_GetPerformanceCounter();
		deltaTime = ((currTime - prevTime) / (double)(SDL_GetPerformanceFrequency()));
		if (framecount == 0) deltaTime = 0.001; // Fake deltatime for the first frame
		time += deltaTime;
		framecount++;
	}
};