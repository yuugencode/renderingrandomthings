#include "Window.h"

#include "Engine/Log.h"

void Window::Create(int width, int height) {
	this->width = width;
	this->height = height;
	sdlPtr = SDL_CreateWindow("Rnd Renderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
	if (sdlPtr == nullptr) LOG_FATAL_AND_EXIT(fmt::runtime(SDL_GetError()));

	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	if (!SDL_GetWindowWMInfo(sdlPtr, &wmInfo)) LOG_FATAL_AND_EXIT(fmt::runtime(SDL_GetError()));
	windowHandle = wmInfo.info.win.window;
	displayHandle = wmInfo.info.win.hdc;
}

void Window::Destroy() {
	if (sdlPtr != nullptr) SDL_DestroyWindow(sdlPtr);
}
