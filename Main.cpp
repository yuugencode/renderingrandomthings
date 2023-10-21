#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <sdl/SDL.h>
#include <glm/glm.hpp>

import Log;
import Utils;
import Window;
import Input;
import ImguiDrawer;
import BgfxCallback;
import Time;
import MiMallocator;

int main(int argc, char* argv[]) {

	// Init SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		Log::Line("SDL Init failed: {}", SDL_GetError());
		return EXIT_FAILURE;
	}

	// Create window
	Window window(1024, 768);
	if (!window.IsValid()) {
		Log::Line("Failed to create window: {}", SDL_GetError());
		SDL_Quit();
		return EXIT_FAILURE;
	}

	// Init Bgfx
	MiMallocator allocator;
	bgfx::Init init;
	init.type = bgfx::RendererType::Direct3D11;
	init.vendorId = BGFX_PCI_ID_NONE;
	init.platformData.nwh = window.windowHandle;
	init.platformData.ndt = window.displayHandle;
	init.resolution.width = window.width;
	init.resolution.height = window.height;
	init.resolution.reset = BGFX_RESET_VSYNC;
	init.callback = CreateBgfxCallback(); // Horror hack, read function comment
	init.allocator = &allocator;

	bgfx::renderFrame(); // Makes bgfx not create a render thread

	if (!bgfx::init(init)) return EXIT_FAILURE;

	// Set view rect to full screen and add clear flags
	const bgfx::ViewId kClearView = 0;
	bgfx::setViewClear(kClearView, BGFX_CLEAR_COLOR, 0xff000000);
	bgfx::setViewRect(kClearView, 0, 0, bgfx::BackbufferRatio::Equal);

	ImguiDrawer::Init(window);

	bool showBgfxStats = false;

	// Create mutable texture
	//auto texture = bgfx::createTexture2D(window.width, window.height, false, 1, bgfx::TextureFormat::RGBA8U, BGFX_SAMPLER_POINT | BGFX_SAMPLER_UVW_CLAMP);
	//auto buffer = bgfx::alloc(window.width * window.height * 4);
	//for (size_t i = 0; i < buffer->size; i++) {
	//	buffer->data[i] = '\xff';
	//}
	//bgfx::updateTexture2D(texture, 0, 0, 0, 0, window.width, window.height, buffer);

	// Main loop
	while (true) {
		
		Time::Tick();

		Input::UpdateKeys();
		
		if (Input::OnKeyDown(SDL_KeyCode::SDLK_ESCAPE)) break;
		if (Input::OnKeyDown(SDL_KeyCode::SDLK_F1)) showBgfxStats = !showBgfxStats;

		bgfx::touch(kClearView); // Triggers viewClear pass
		bgfx::dbgTextClear();
		
		ImguiDrawer::NewFrame(window, (float)Time::deltaTime);
		ImguiDrawer::TestDraws();
		ImguiDrawer::Render();

		const bgfx::Stats* stats = bgfx::getStats();
		
		Log::Screen(0, "F1 to toggle stats");
		Log::Screen(1, "Backbuffer {}W x {}H in pixels, debug text {}W x {}H in characters.", stats->width, stats->height, stats->textWidth, stats->textHeight);
		Log::Screen(2, "Deltatime: {}", Time::deltaTime);
		Log::Screen(3, "Time: {}", Time::time);
		Log::Screen(4, "MouseLeft: {}", Input::MouseHeld(1));
		Log::Screen(5, "MouseMid: {}", Input::MouseHeld(2));
		Log::Screen(6, "MouseRight: {}", Input::MouseHeld(3));

		bgfx::setDebug(showBgfxStats ? BGFX_DEBUG_STATS : BGFX_DEBUG_TEXT);
		
		bgfx::frame();
	}

	// Exit cleanup
	ImguiDrawer::Quit();
	SDL_Quit();
	//bgfx::destroy(texture);
	if (init.callback != nullptr) delete init.callback; // Hack, see CreateBgfxCallback() comment

	return EXIT_SUCCESS;
}
