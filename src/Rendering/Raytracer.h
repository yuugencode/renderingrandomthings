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

	static Raytracer* Instance;

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
	std::vector<glm::vec4> bvhBuffer;

	// 1920 1080 common factors
	// 1 | 2 | 3 | 4 | 5 | 6 | 8 | 10 | 12 | 15 | 20 | 24 | 30 | 40 | 60 | 120
	const uint32_t bvhBufferDiv = 4;

	// Shortlisted bvhBuffer from which shadowbvh is generated
	std::vector<glm::vec4> shadowBuffer;

	// Bvh sampled for smooth shadows
	BvhPoint shadowBvh;

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
	Timer shadowTimerSample;
	Timer shadowTimerGen;

	// Initializes a new raytracer for given window
	void Create(const Window& window);

	// Shoots a ray against the scene and returns the result
	RayResult RaycastScene(const Scene& scene, const Ray& ray) const;
	
	// Traces a ray against the scene and returns the color for whatever it hit
	glm::vec4 TraceRay(const Scene& scene, const Ray& ray, int& recursionDepth) const;

	// Renders given scene to a texture and blits on screen
	void RenderScene(const Scene& scene);

	~Raytracer() {
		bgfx::destroy(texture);
		bgfx::destroy(u_texture);
		delete textureBuffer;
	}
};