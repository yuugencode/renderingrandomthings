#pragma once

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Boilerplate/Window.h"
#include "Engine/Utils.h"
#include "Engine/Timer.h"
#include "Engine/BvhPoint.h"
#include "Game/Entity.h"
#include "Game/Scene.h"
#include "Rendering/RayResult.h"

// Raytracer for a given scene
class Raytracer {

public:

	// bgfx Rendering layer
	const bgfx::ViewId VIEW_LAYER = 0;

	// The texture the raytracer updates
	bgfx::TextureHandle texture;
	bgfx::UniformHandle u_texture;
	bgfx::TextureFormat::Enum textureFormat = bgfx::TextureFormat::RGBA8;
	
	// GPU Shader (just blits an array on the screen)
	bgfx::ProgramHandle program; 

	// Texture data buffer
	Color* textureBuffer;
	uint32_t textureBufferSize;

	// "Backbuffer" used during shadow accelerator sampling
	std::vector<glm::vec4> screenTempBuffer;
	std::vector<glm::vec4> indirectBuffer;

	// Common divisors
	// 1920x1080: 1 | 2 | 3 | 4 | 5 | 6 | 8 | 10 | 12 | 15 | 20 | 24 | 30 | 40 | 60 | 120
	// 1280x720: 1 | 2 | 4 | 5 | 8 | 10 | 16 | 20 | 40 | 80
	// 800x600: 1 | 2 | 4 | 5 | 8 | 10 | 20 | 25 | 40 | 50 | 100 | 200
	// 640x480: 1 | 2 | 4 | 5 | 8 | 10 | 16 | 20 | 32 | 40 | 80 | 160

	const uint32_t screenTempBufferDiv = 4;

	// Vertex layout for the full screen pass
	struct PosColorTexCoord0Vertex {
		glm::vec3 pos;
		Color rgba;
		glm::vec2 uv;
	};
	bgfx::VertexLayout vtxLayout;

	// Current window
	const Window* window;

	Timer traceTimer;
	Timer lightBufferSampleTimer;
	Timer lightBufferGenTimer;

	// Initializes a new raytracer for given window
	void Create(const Window& window);

	glm::vec4 GetColor(const Scene& scene, const RayResult& rayResult, const Ray& ray, TraceData& opts) const;

	// Shoots a ray against the scene and returns the result
	RayResult RaycastScene(const Scene& scene, const Ray& ray) const;
	
	// Traces a ray against the scene and returns the color for whatever it hit
	glm::vec4 TraceRay(const Scene& scene, const Ray& ray, TraceData& opts) const;

	// Renders given scene to a texture and blits on screen
	void RenderScene(Scene& scene);

	~Raytracer() {
		bgfx::destroy(texture);
		bgfx::destroy(u_texture);
		delete textureBuffer;
	}
};
