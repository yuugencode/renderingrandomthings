#pragma once

#include <sdl/SDL.h>

#include <vector>

// Game time related things
class Time {
	Time() = default;
	static Uint64 prevTime;
	static Uint64 currTime;
	static std::vector<double> deltaTimes;
	static size_t deltaTimesI;

public:
	static Uint64 framecount; // How many frames have been rendered since start
	
	static double deltaTime; // Frame delta
	static float deltaTimeF; // Frame delta
	
	static double time; // Running time
	static float timeF; // Running time
	
	static double timeSpeed; // Multiplier for global time

	static double smoothDeltaTime; // Smoothed delta time over several samples

	// Updates current time values, should be called once at the beginning of a frame
	static void Tick();

	// Returns the current accurate time, not tied to frame ticks 
	static double GetAccurateTime();
};
