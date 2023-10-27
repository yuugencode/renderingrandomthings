#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <sdl/SDL.h>
#include <glm/glm.hpp>
#include <sdl/SDL.h>

import <filesystem>;
import MiAllocator;
import ImguiDrawer;
import BgfxCallback;
import Raytracer;
import Game;
import Timer;
import Mesh;

int main(int argc, char* argv[]) {

	// Init SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		Log::Line("SDL Init failed: {}", SDL_GetError());
		return EXIT_FAILURE;
	}

	// Create window
	Game::window.Create(1024, 768);

	// Init Bgfx
	MiAllocator allocator;
	bgfx::Init init;
	init.type = bgfx::RendererType::Direct3D11;
	init.vendorId = BGFX_PCI_ID_NONE;
	init.platformData.nwh = Game::window.windowHandle;
	init.platformData.ndt = Game::window.displayHandle;
	init.resolution.width = Game::window.width;
	init.resolution.height = Game::window.height;
	init.resolution.reset = BGFX_RESET_VSYNC | BGFX_RESET_FLIP_AFTER_RENDER | BGFX_RESET_FLUSH_AFTER_RENDER;
	init.callback = CreateBgfxCallback(); // Hack, read function comment
	init.allocator = &allocator;

	bgfx::renderFrame(); // Make bgfx not create a render thread

	if (!bgfx::init(init)) return EXIT_FAILURE;

	// Set view rect to full screen and add clear flags
	const bgfx::ViewId MAIN_VIEW = 0;
	bgfx::setViewClear(MAIN_VIEW, BGFX_CLEAR_COLOR, 0x33333300);
	bgfx::setViewRect(MAIN_VIEW, 0, 0, bgfx::BackbufferRatio::Equal);

	// Imgui
	ImguiDrawer::Init();

	// Add something to scene
	Raytracer raytracer;
	Game::rootScene.entities.push_back(std::make_unique<Plane>(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
	Game::rootScene.entities.push_back(std::make_unique<Sphere>(glm::vec3(0.0f, 0.0f, 0.0f), 2.0f));
	
	auto mesh = std::make_shared<Mesh>(std::filesystem::path("ext/char.fbx"));
	Game::rootScene.entities.push_back(std::make_unique<RenderedMesh>(mesh));

	Timer raytraceTimer(64);
	
	bool showBgfxStats = false;

	// Main loop
	while (true) {
		
		Time::Tick();
		
		Input::UpdateKeys();
		
		if (Input::OnKeyDown(SDL_KeyCode::SDLK_ESCAPE) || Input::OnKeyDown(SDL_KeyCode::SDLK_RETURN)) break;
		if (Input::OnKeyDown(SDL_KeyCode::SDLK_F1)) showBgfxStats = !showBgfxStats;

		bgfx::touch(MAIN_VIEW); // Triggers viewClear pass
		bgfx::dbgTextClear();
		
		ImguiDrawer::NewFrame();
		ImguiDrawer::TestDraws();
		ImguiDrawer::Render();

		const bgfx::Stats* stats = bgfx::getStats();
		
		Log::Screen(0, "F1 to toggle stats");
		Log::Screen(1, "Backbuffer: {}W x {}H", stats->width, stats->height);
		Log::Screen(2, "Deltatime: {}, FPS: {}", Time::deltaTime, 1.0 / Time::smoothDeltaTime);
		Log::Screen(3, "Time: {}", Time::time);
		Log::Screen(4, "MouseLeft: {}", Input::MouseHeld(1));
		Log::Screen(5, "MouseMid: {}", Input::MouseHeld(2));
		Log::Screen(6, "MouseRight: {}", Input::MouseHeld(3));

		// Trace the scene and print averaged time it took
		raytraceTimer.Start();
		raytracer.TraceScene();
		raytraceTimer.End();
		Log::Screen(7, "Tracing (ms): {}", raytraceTimer.GetAveragedTime() * 1000.0);

		bgfx::setDebug(showBgfxStats ? BGFX_DEBUG_STATS : BGFX_DEBUG_TEXT);
		
		bgfx::frame();
	}

	// Exit cleanup
	ImguiDrawer::Quit();
	Game::window.Destroy();
	SDL_Quit();
	if (init.callback != nullptr) delete init.callback; // Hack, see CreateBgfxCallback() comment

	return EXIT_SUCCESS;
}
