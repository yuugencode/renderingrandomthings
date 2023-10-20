module;

#include <sdl/SDL.h>
#include <sdl/SDL_syswm.h>

export module Window;

import Log;

/// <summary> Simple wrapper for SDL_Window </summary>
export class Window {

public:

	SDL_Window* sdlPtr;
	HWND windowHandle;
	HDC displayHandle;

	int width, height;

	Window(int width, int height) {
		this->width = width;
		this->height = height;
		sdlPtr = SDL_CreateWindow("Rnd Renderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
		if (IsValid()) GetHandles(windowHandle, displayHandle); // Autopopulate window/device handles
	}

	bool IsValid() { return sdlPtr != nullptr; }

	void GetHandles(HWND& window, HDC& device) {
		// Init bgfx and pass sdl window to it
		SDL_SysWMinfo wmInfo;
		SDL_VERSION(&wmInfo.version);
		SDL_GetWindowWMInfo(sdlPtr, &wmInfo);
		window = wmInfo.info.win.window;
		device = wmInfo.info.win.hdc;
	}

	~Window() {
		if (sdlPtr != nullptr) SDL_DestroyWindow(sdlPtr);
	}
};