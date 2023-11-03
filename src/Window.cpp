#include "Window.h"

#include "Log.h"

void Window::Create(int width, int height) {
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

void Window::Destroy() {
	if (sdlPtr != nullptr) SDL_DestroyWindow(sdlPtr);
}
