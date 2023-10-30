#define GLM_FORCE_LEFT_HANDED 1

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <sdl/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <sdl/SDL.h>

import <algorithm>;
import MiAllocator;
import ImguiDrawer;
import BgfxCallback;
import Raytracer;
import Game;
import Timer;
import Mesh;
import Shapes;
import RenderedMesh;
import Utils;

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
	init.resolution.reset = BGFX_RESET_VSYNC;// | BGFX_RESET_FLIP_AFTER_RENDER | BGFX_RESET_FLUSH_AFTER_RENDER;
	//init.resolution.reset = BGFX_RESET_NONE;
	init.resolution.maxFrameLatency = 1; // Some bug somewhere makes the rendering jittery if this is not set
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

	// Camera
	Game::scene.camera = {
		.transform = { 
			.position = glm::vec3(2.0f, 1.0f + 1.0f, 3.0f) * 0.5f, 
			.rotation = glm::quatLookAt(glm::normalize(-glm::vec3(2.0f, 1.0f, 3.0f)), glm::vec3(0,1,0)),
			.scale = glm::vec3(1,1,1) },
		.fov = 70.0f,
		.nearClip = 0.05f,
		.farClip = 1000.0f,
	};

	auto& camTf = Game::scene.camera.transform; // Shorter alias

	// Create renderer
	Game::raytracer.Create(Game::window);

	// Add stuff to the scene
	// Parametric shapes
	Game::scene.entities.push_back(std::make_unique<Disk>(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 3.0f));
	Game::scene.entities.push_back(std::make_unique<Sphere>(glm::vec3(2.0f, 0.5f, 0.0f), 0.5f));
	Game::scene.entities.push_back(std::make_unique<Box>(glm::vec3(-2.0f, 0.5f, 0.0f), 0.5f));
	
	// Mesh 1
	auto mesh = std::make_shared<Mesh>(std::filesystem::path("ext/char.fbx"));
	mesh->RotateVertices( glm::quat(glm::vec3( glm::radians(-90.0f), 0.0f, 0.0f)));

	auto rendMesh = std::make_unique<RenderedMesh>(mesh);
	rendMesh->GenerateBVH();
	Game::scene.entities.push_back(std::move(rendMesh));

	// Mesh 2
	auto mesh2 = std::make_shared<Mesh>(std::filesystem::path("ext/dragon.obj"));
	mesh2->ScaleVertices(0.01f);
	mesh2->OffsetVertices(glm::vec3(0, 0.5f, 0));
	
	auto rendMesh2 = std::make_unique<RenderedMesh>(mesh2);
	rendMesh2->GenerateBVH();
	Game::scene.entities.push_back(std::move(rendMesh2));

	// Mesh 3
	auto mesh3 = std::make_shared<Mesh>(std::filesystem::path("ext/rock.fbx"));
	mesh3->ScaleVertices(0.1f);

	auto rendMesh3 = std::make_unique<RenderedMesh>(mesh3);
	rendMesh3->GenerateBVH();
	Game::scene.entities.push_back(std::move(rendMesh3));

	bool showBgfxStats = false;

	// Main loop
	while (true) {
		
		Time::Tick();
		
		Input::UpdateKeys();
		
		// Clear view
		bgfx::touch(MAIN_VIEW);

		// Imgui
		ImguiDrawer::NewFrame();
		ImguiDrawer::DrawUI();

		// System keys
		if (Input::OnKeyDown(SDL_KeyCode::SDLK_ESCAPE) || Input::OnKeyDown(SDL_KeyCode::SDLK_RETURN)) break;
		if (Input::OnKeyDown(SDL_KeyCode::SDLK_F1)) showBgfxStats = !showBgfxStats;

		// Camera movement
		auto offset = glm::vec3(0, 0, 0);
		if (Input::KeyHeld(SDL_KeyCode::SDLK_w)) offset += glm::vec3(0,0, 1);
		if (Input::KeyHeld(SDL_KeyCode::SDLK_s)) offset += glm::vec3(0,0,-1);
		if (Input::KeyHeld(SDL_KeyCode::SDLK_a)) offset += glm::vec3(-1,0,0);
		if (Input::KeyHeld(SDL_KeyCode::SDLK_d)) offset += glm::vec3( 1,0,0);
		if (Input::KeyHeld(SDL_KeyCode::SDLK_e)) offset += glm::vec3(0, 1,0);
		if (Input::KeyHeld(SDL_KeyCode::SDLK_q)) offset += glm::vec3(0,-1,0);
		if (Input::KeyHeld(SDL_KeyCode::SDLK_LSHIFT)) offset *= 0.2f;
		camTf.position += camTf.rotation * offset * Time::deltaTimeF * 4.0f;

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

		// Sort scene objects
		std::ranges::sort(Game::scene.entities, [&](const std::unique_ptr<Entity>& a, const std::unique_ptr<Entity>& b) {
			return a->EstimatedDistanceTo(camTf.position) < b->EstimatedDistanceTo(camTf.position);
		});

		// Trace the scene
		Game::raytracer.TraceScene(Game::scene);

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
