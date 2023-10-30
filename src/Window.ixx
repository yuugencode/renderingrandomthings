module;

#include <sdl/SDL.h>
#include <sdl/SDL_syswm.h>
#include <cassert>
#include <memory>

export module Window;

import Log;

/// <summary> Simple wrapper for SDL_Window </summary>
export class Window {
public:
	Window() = default;

	SDL_Window* sdlPtr;
	HWND windowHandle;
	HDC displayHandle;
	uint32_t width, height;

	void Create(int width, int height) {
		this->width = width;
		this->height = height;
		sdlPtr = SDL_CreateWindow("Rnd Renderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
		if (sdlPtr == nullptr) Log::FatalError(SDL_GetError());

		SDL_SysWMinfo wmInfo;
		SDL_VERSION(&wmInfo.version);
		if (!SDL_GetWindowWMInfo(sdlPtr, &wmInfo)) Log::FatalError(SDL_GetError());
		windowHandle = wmInfo.info.win.window;
		displayHandle = wmInfo.info.win.hdc;
	}

	// The dinosaur C-style API of the graphics libraries doesn't really play well with C++20 modules and/or smart pointers
	// Instead of writing a lengthy boilerplate wrapper just use explicit destroy calls for these
	void Destroy() {
		if (sdlPtr != nullptr) SDL_DestroyWindow(sdlPtr);
	}

	// Holding pointers so delete copy/assigns to avoid double frees
	Window(const Window&) = delete;
	Window& operator= (const Window&) = delete;
};