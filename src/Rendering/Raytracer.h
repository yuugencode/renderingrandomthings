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

	// Profiling timers
	Timer sceneTraceTimer, lightBufferSampleTimer, lightBufferGenTimer, indirectSampleTimer, indirectGenTimer;

	// Initializes a new raytracer for given window
	void Create(const Window& window);

	// Shoots a ray against the scene and returns information about what we hit if anything
	RayResult RaycastScene(const Scene& scene, const Ray& ray) const;

	// Traces a ray against the scene recursively and returns the color for whatever it hit
	glm::vec4 TracePath(const Scene& scene, const Ray& ray, TraceData& opts) const;

	// Given raycast results samples a shader and returns the expected color at given position
	glm::vec4 SampleColor(const Scene& scene, const RayResult& rayResult, const Ray& ray, TraceData& opts) const;
	
	// Renders given scene to a texture and blits on screen
	void RenderScene(Scene& scene);

	~Raytracer() {
		bgfx::destroy(texture);
		bgfx::destroy(u_texture);
		delete textureBuffer;
	}

private:

	// Calculates indirect lighting into each light's own BVH
	void IndirectLightingPass(Scene& scene, const glm::mat4x4& projInv, const glm::mat4x4& viewInv);

	// Calculates screen space areas that are lit and saves it to each light's own BVH
	void SmoothShadowsPass(Scene& scene, const glm::mat4x4& projInv, const glm::mat4x4& viewInv);

	// Calculates the main per pixel lighting for the scene
	void MainDirectPass(Scene& scene, const glm::mat4x4& projInv, const glm::mat4x4& viewInv);

	// Temp buffer used during shadow accelerator sampling
	std::vector<glm::vec4> screenTempBuffer;

	// The texture the raytracer updates
	bgfx::TextureHandle texture;
	bgfx::UniformHandle u_texture;
	bgfx::TextureFormat::Enum textureFormat = bgfx::TextureFormat::RGBA8;

	// GPU Shader (just blits an array on the screen)
	bgfx::ProgramHandle program;

	// Texture data buffer
	Color* textureBuffer;
	uint32_t textureBufferSize;

	// Current window
	const Window* window;

	// Vertex layout for the full screen pass
	struct PosColorTexCoord0Vertex {
		glm::vec3 pos;
		Color rgba;
		glm::vec2 uv;
	};
	bgfx::VertexLayout vtxLayout;
};
