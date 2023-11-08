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
	init.resolution.reset = BGFX_RESET_VSYNC;// | BGFX_RESET_FLIP_AFTER_RENDER | BGFX_RESET_FLUSH_AFTER_RENDER;
	//init.resolution.reset = BGFX_RESET_NONE;
	init.resolution.maxFrameLatency = 1; // Some bug somewhere makes the rendering jittery if this is not set
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

	// Camera
	const auto camStartPos = glm::vec3(10.0f, 10.0f, 10.0f) * 0.5f;
	Game::scene.camera = {
		.transform = { 
			.position = camStartPos,
			.rotation = glm::normalize(glm::quatLookAt(glm::normalize(camStartPos), glm::vec3(0,1,0))),
			.scale = glm::vec3(1,1,1) },
		.fov = 70.0f,
		.nearClip = 0.05f,
		.farClip = 1000.0f,
	};

	auto& camTf = Game::scene.camera.transform; // Shorter alias

	// Lights
	Game::scene.lights.push_back(Light{ .position = glm::vec3(4.0f, 7.5f,  7.0f), .color = glm::vec3(1.0f, 0.9f, 0.7f), .range = 15.0f, .intensity = 1.5f });
	Game::scene.lights.push_back(Light{ .position = glm::vec3(4.0f, 5.5f, -7.0f), .color = glm::vec3(0.8f, 1.0f, 1.0f), .range = 15.0f, .intensity = 1.5f });

	// Random parametric shapes
	Game::scene.entities.push_back(std::make_unique<Disk>(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 20.0f));

	Game::scene.entities.push_back(std::make_unique<Sphere>(glm::vec3(1.0f, 0.5f, 0.0f), 0.5f));
	Game::scene.entities.back()->reflectivity = 0.3f;
	Game::scene.entities.push_back(std::make_unique<Sphere>(glm::vec3(0.0f, 1.5f, -5.0f), 1.5f));
	Game::scene.entities.back()->reflectivity = 0.5f;
	Game::scene.entities.push_back(std::make_unique<Sphere>(glm::vec3(1.0f, 0.5f, 5.0f), 0.5f));
	Game::scene.entities.back()->reflectivity = 0.7f;
	
	Game::scene.entities.push_back(std::make_unique<Box>(glm::vec3(-2.0f, 0.5f, 0.0f), glm::vec3(1.0f, 4.0f, 0.05f)));
	Game::scene.entities.back()->reflectivity = 0.5f;
	auto box = Game::scene.entities.back().get();

	// Mesh 1
	auto meshHandle = Assets::NewMesh(std::filesystem::path("ext/char.fbx"));
	auto rendMesh = std::make_unique<RenderedMesh>(meshHandle);
	
	// Hardcoded mesh 1 textures, kind of like materials without materials
	rendMesh->textureHandles.push_back(Assets::NewTexture(std::filesystem::path("ext/tex1.png"), true));
	rendMesh->textureHandles.push_back(Assets::NewTexture(std::filesystem::path("ext/tex4.png"), true));
	rendMesh->textureHandles.push_back(Assets::NewTexture(std::filesystem::path("ext/tex3.png"), true));
	rendMesh->textureHandles.push_back(Assets::NewTexture(std::filesystem::path("ext/tex3.png"), true));
	rendMesh->textureHandles.push_back(Assets::NewTexture(std::filesystem::path("ext/tex5.png"), true));
	rendMesh->textureHandles.push_back(Assets::NewTexture(std::filesystem::path("ext/tex2.png"), true));
	rendMesh->textureHandles.push_back(Assets::NewTexture(std::filesystem::path("ext/tex3.png"), true));
	
	auto chara = rendMesh.get();

	rendMesh->transform.rotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1,0,0));
	Game::scene.entities.push_back(std::move(rendMesh));

	// Mesh 2
	auto meshHandle2 = Assets::NewMesh(std::filesystem::path("ext/dragon.obj"));
	
	auto rendMesh2 = std::make_unique<RenderedMesh>(meshHandle2);
	Game::scene.entities.push_back(std::move(rendMesh2));

	auto dragon = Game::scene.entities.back().get();
	dragon->transform.scale = glm::vec3(1, 1, 1) * 0.02f;
	dragon->transform.position += glm::vec3(4.0f, 0.8f, 0.0f);
	dragon->transform.LookAtDir(glm::vec3(-1, 0, 0), glm::vec3(0, 1, 0));
	dragon->shaderType = RenderedMesh::Shader::PlainWhite;

	// Mesh 3
	//auto meshHandle3 = Assets::NewMesh(std::filesystem::path("ext/rock.fbx"));
	//Assets::Meshes[meshHandle3]->ScaleVertices(0.1f);
	//
	//auto rendMesh3 = std::make_unique<RenderedMesh>(meshHandle3);
	//rendMesh3->GenerateBVH();
	//Game::scene.entities.push_back(std::move(rendMesh3));

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
		if (Input::OnKeyDown(SDL_KeyCode::SDLK_t)) Time::timeSpeed = Time::timeSpeed > 0.5 ? 0.1 : 1.0;
		if (Input::OnKeyDown(SDL_KeyCode::SDLK_y)) Time::timeSpeed = Time::timeSpeed > 0.05 ? 0.001 : 1.0;

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
		chara->transform.position += glm::vec3(0, 1, 0) * glm::sin(Time::timeF * 2.0f) * 0.01f;
		chara->transform.rotation = glm::angleAxis(Time::deltaTimeF * 2.0f, glm::vec3(0, 1, 0)) * chara->transform.rotation;
		chara->transform.rotation = glm::normalize(chara->transform.rotation);

		dragon->transform.rotation = glm::angleAxis(-Time::deltaTimeF * 2.0f, glm::vec3(0, 1, 0)) * dragon->transform.rotation;
		dragon->transform.rotation = glm::normalize(dragon->transform.rotation);
		dragon->transform.scale = glm::vec3(0.002f) + glm::abs(glm::sin(Time::timeF) * 0.8f * 0.02f);

		box->transform.rotation = glm::angleAxis(-Time::deltaTimeF * 0.1f, glm::vec3(0, 1, 0)) * box->transform.rotation;
		box->transform.rotation = glm::normalize(box->transform.rotation);

		for (int i = 0; i < Game::scene.lights.size(); i++) {
			Light& light = Game::scene.lights[i];
			light.position = glm::normalize(glm::angleAxis(Time::deltaTimeF * 0.6f * Utils::Hash11((float)(i + 33)), glm::vec3(0, 1, 0))) * light.position;
		}

		// Update matrices
		for (const auto& obj : Game::scene.entities) {
			obj->invModelMatrix = glm::inverse(obj->transform.ToMatrix());
		}

		// Trace the scene
		Game::raytracer.RenderScene(Game::scene);

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
