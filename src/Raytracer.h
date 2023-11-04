#pragma once

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <ppl.h> // Parallel for

#include "Utils.h"
#include "Entity.h"
#include "Window.h"
#include "Timer.h"
#include "Scene.h"
#include "BvhPoint.h"

// Raycast result
struct RayResult {
	glm::vec3 localPos;
	glm::vec3 localNormal;
	Entity* obj;
	float depth;
	uint32_t data;
	bool Hit() const { return obj != nullptr; }
};

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

	std::vector<glm::vec3> bvhBuffer;
	const uint32_t bvhBufferDiv = 16;

	std::vector<float> shadowBuffer;
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

	Timer timer;

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
