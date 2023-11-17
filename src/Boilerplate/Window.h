#pragma once

#include <sdl/SDL.h>
#include <sdl/SDL_syswm.h>

// Simple wrapper for SDL_Window 
class Window {
public:
	Window() = default;

	SDL_Window* sdlPtr;
	HWND windowHandle;
	HDC displayHandle;
	int width, height;

	void Create(int width, int height);
	void Destroy();

	// Holding pointers so delete copy/assigns to avoid double frees
	Window(const Window&) = delete;
	Window& operator= (const Window&) = delete;
};
