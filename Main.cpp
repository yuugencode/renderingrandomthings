#define GLM_FORCE_LEFT_HANDED 1
#define SDL_MAIN_HANDLED 1

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <sdl/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <sdl/SDL.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <ppl.h> // Parallel for

#include "Boilerplate/MiAllocator.h"
#include "Boilerplate/BgfxCallback.h"
#include "Boilerplate/ImguiDrawer.h"
#include "Engine/Utils.h"
#include "Engine/Time.h"
#include "Engine/Input.h"
#include "Engine/Log.h"
#include "Engine/Assets.h"
#include "Rendering/Light.h"
#include "Game/Entity.h"
#include "Game/RenderedMesh.h"
#include "Game/Shapes.h"
#include "Game/Game.h"

int main(int argc, char* argv[]) {

	// Init SDL
	SDL_SetMainReady();

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		Log::Line("SDL Init failed: {}", SDL_GetError());
		return EXIT_FAILURE;
	}

	// Create window
	Game::window.Create(1280, 720);

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
	//init.resolution.reset = BGFX_RESET_NONE; // Uncapped fps
	init.resolution.maxFrameLatency = 1;
	init.callback = new BgfxCallback();
	init.allocator = &allocator;

	bgfx::renderFrame(); // Make bgfx not create a render thread

	if (!bgfx::init(init)) return EXIT_FAILURE;

	// Set view rect to full screen and add clear flags
	const bgfx::ViewId MAIN_VIEW = 0;
	bgfx::setViewClear(MAIN_VIEW, BGFX_CLEAR_COLOR, 0x33333300);
	bgfx::setViewRect(MAIN_VIEW, 0, 0, bgfx::BackbufferRatio::Equal);

	// Imgui
	ImguiDrawer::Init();

	// Create renderer
	Game::raytracer.Create(Game::window);

	// Add stuff to the scene
	Game::scene.ReadAndAddTestObjects();

	// Camera
	const auto camStartPos = glm::vec3(-1.5f, 3.7f, 5.6f);
	//const auto camStartPos = glm::vec3(7.0f, 6.0f, 0.0f);
	Game::scene.camera = {
		.transform = { 
			.position = camStartPos,
			.rotation = glm::normalize(glm::quatLookAt(glm::normalize(camStartPos), glm::vec3(0,1,0))),
			.scale = glm::vec3(1,1,1) },
		.fov = 70.0f,
		.nearClip = 0.05f,
		.farClip = 1000.0f,
	};

	bool showBgfxStats = false, paused = false;

	// Main loop
	while (true) {
		
		Time::Tick();
		Input::UpdateKeys();

		// Loop in place if paused, useful for saving CPU while staring at the screen in thought
		if (Input::OnKeyDown(SDL_KeyCode::SDLK_SPACE)) paused = !paused;
		if (paused) { SDL_Delay(10); continue; }

		// Clear view
		if (!paused) bgfx::touch(MAIN_VIEW);

		// Imgui
		ImguiDrawer::NewFrame();
		ImguiDrawer::DrawUI();

		// System keys
		if (Input::OnKeyDown(SDL_KeyCode::SDLK_ESCAPE) || Input::OnKeyDown(SDL_KeyCode::SDLK_RETURN)) break;
		if (Input::OnKeyDown(SDL_KeyCode::SDLK_F1)) showBgfxStats = !showBgfxStats;
		if (Input::OnKeyDown(SDL_KeyCode::SDLK_t)) Time::timeSpeed = Time::timeSpeed > 0.5 ? 0.1 : 1.0;
		if (Input::OnKeyDown(SDL_KeyCode::SDLK_y)) Time::timeSpeed = Time::timeSpeed > 0.05 ? 0.00001 : 1.0;

		// Camera movement
		auto offset = glm::vec3(0, 0, 0);
		if (Input::KeyHeld(SDL_KeyCode::SDLK_w)) offset += glm::vec3(0,0, 1);
		if (Input::KeyHeld(SDL_KeyCode::SDLK_s)) offset += glm::vec3(0,0,-1);
		if (Input::KeyHeld(SDL_KeyCode::SDLK_a)) offset += glm::vec3(-1,0,0);
		if (Input::KeyHeld(SDL_KeyCode::SDLK_d)) offset += glm::vec3( 1,0,0);
		if (Input::KeyHeld(SDL_KeyCode::SDLK_e)) offset += glm::vec3(0, 1,0);
		if (Input::KeyHeld(SDL_KeyCode::SDLK_q)) offset += glm::vec3(0,-1,0);
		if (Input::KeyHeld(SDL_KeyCode::SDLK_LSHIFT)) offset *= 0.2f;
		
		auto& camTf = Game::scene.camera.transform; // Shorter alias
		camTf.position += camTf.rotation * offset * Time::unscaledDeltaTimeF * 4.0f;

		// Press R to drop light at current pos
		if (Input::OnKeyDown(SDL_KeyCode::SDLK_r))
			Game::scene.lights.back().position = camTf.position;

		// Mouse rotation on right click
		static int clickPos[2];
		if (Input::OnMouseDown(SDL_BUTTON_RIGHT)) SDL_SetRelativeMouseMode(SDL_TRUE);
		if (Input::OnMouseUp(SDL_BUTTON_RIGHT)) SDL_SetRelativeMouseMode(SDL_FALSE);
		if (Input::MouseHeld(SDL_BUTTON_RIGHT)) {
			auto xy = glm::vec2(Input::mouseDelta[0], Input::mouseDelta[1]) * 0.0075f;
			auto os1 = glm::angleAxis(xy.x, glm::vec3(0,1,0));
			auto os2 = glm::angleAxis(xy.y, glm::vec3(1,0,0));
			camTf.rotation = (os1 * camTf.rotation) * os2;
		}

		// Bgfx debug info
		bgfx::dbgTextClear();
		bgfx::setDebug(showBgfxStats ? BGFX_DEBUG_STATS : BGFX_DEBUG_TEXT);

		// Temp debug movement for things
		Entity* bunny = Game::scene.GetObjectByName("bunny");
		if (bunny != nullptr) {
			bunny->transform.rotation = glm::angleAxis(Time::deltaTimeF * 2.0f, glm::vec3(0, 1, 0)) * bunny->transform.rotation;
			bunny->transform.rotation = glm::normalize(bunny->transform.rotation);
		}
		Entity* ball = Game::scene.GetObjectByName("ball");
		if (ball != nullptr) {
			ball->transform.rotation = glm::angleAxis(-Time::deltaTimeF * 1.0f, glm::vec3(0, 0, 1)) * ball->transform.rotation;
			ball->transform.rotation = glm::normalize(ball->transform.rotation);
		}

		// Move lights
		for (int i = 0; i < Game::scene.lights.size(); i++) {
			Light& light = Game::scene.lights[i];
			//light.position = glm::normalize(glm::angleAxis(Time::deltaTimeF * 0.6f * Utils::Hash11((float)(i + 33)), glm::vec3(0, 1, 0))) * light.position;
			glm::vec3 defPos = glm::vec3(-5.0f, 5.0f, 0.0f);
			light.position = defPos + 
				glm::vec3(5, 0, 0) * (float)i +
				glm::vec3(0,4.9,0) * glm::sin(Time::timeF * (float)(i+1) * 0.25f) +
				glm::vec3(0,0,2) * glm::sin(Time::timeF * (float)(i+1) * 0.3f);
		}

		// Update object matrices @TODO: Caching, transform hierarchies etc etc.. maybe one day
		concurrency::parallel_for(size_t(0), Game::scene.entities.size(), [&](size_t i) {
			auto& obj = Game::scene.entities[i];
			obj->modelMatrix = obj->transform.ToMatrix();
			obj->invModelMatrix = glm::inverse(obj->modelMatrix);
			
			// Calculate rotated AABB for every obj
			glm::mat4x4 lowPts =  { obj->aabb.GetVertice(0), obj->aabb.GetVertice(1), obj->aabb.GetVertice(2), obj->aabb.GetVertice(3), };
			lowPts = obj->modelMatrix * lowPts;
			glm::mat4x4 highPts = { obj->aabb.GetVertice(4), obj->aabb.GetVertice(5), obj->aabb.GetVertice(6), obj->aabb.GetVertice(7), };
			highPts = obj->modelMatrix * highPts;
			AABB globalAABB = AABB(lowPts[0], lowPts[0]);
			for (int i = 0; i < 4; i++) { globalAABB.Encapsulate(lowPts[i]); globalAABB.Encapsulate(highPts[i]); }
			obj->worldAABB = globalAABB;
		});

		// Trace the scene
		Game::raytracer.RenderScene(Game::scene);

		// Draw UI
		ImguiDrawer::Render();

		// Dump on screen
		bgfx::frame();
	}

	// Exit cleanup
	ImguiDrawer::Quit();
	Game::window.Destroy();
	SDL_Quit();
	if (init.callback != nullptr) delete init.callback; // Hack, see CreateBgfxCallback() comment

	return EXIT_SUCCESS;
}
