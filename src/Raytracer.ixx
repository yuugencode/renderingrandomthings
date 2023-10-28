module;

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <ppl.h> // Parallel for

export module Raytracer;

import Game;
import Shapes;
import <memory>;
import <filesystem>;
import <algorithm>;

export class Raytracer {

public:

	// bgfx Rendering layer
	const bgfx::ViewId VIEW_LAYER = 0;

	// The texture the raytracer updates
	bgfx::TextureHandle texture;
	bgfx::UniformHandle u_texture;
	bgfx::TextureFormat::Enum textureFormat = bgfx::TextureFormat::RGBA8;
	
	bgfx::ProgramHandle program; // Shader

	// Texture data buffer
	Color* textureBuffer;
	uint32_t textureBufferSize;

	// Vertex layout for full screen pass
	struct PosColorTexCoord0Vertex {
		glm::vec3 pos;
		Color rgba;
		glm::vec2 uv;
	};
	bgfx::VertexLayout vtxLayout;

	Raytracer() {

		vtxLayout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.end();

		// Create mutable texture
		texture = bgfx::createTexture2D(Game::window.width, Game::window.height, false, 1, textureFormat);
		
		bgfx::TextureInfo info;
		bgfx::calcTextureSize(info, Game::window.width, Game::window.height, 1, false, false, 1, textureFormat);
		textureBufferSize = info.storageSize;
		textureBuffer = new Color[textureBufferSize]; // (Color*)malloc(textureBufferSize);

		u_texture = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

		// Read and compile shaders
		const std::filesystem::path folder = "shaders/";
		auto vShader = Utils::LoadShader(folder / "screenpass_vs.bin");
		auto fShader = Utils::LoadShader(folder / "screenpass_fs.bin");
		assert(vShader.idx != bgfx::kInvalidHandle);
		assert(fShader.idx != bgfx::kInvalidHandle);

		program = bgfx::createProgram(vShader, fShader, true);
		assert(program.idx != bgfx::kInvalidHandle);

		// Full screen pass is a single triangle so clip anything outside
		bgfx::setViewRect(VIEW_LAYER, 0, 0, bgfx::BackbufferRatio::Equal);
	}

	Color TraceRay(const glm::vec3& pos, const glm::vec3& dir) {
		
		Color output;
		float minDepth = std::numeric_limits<float>::max();

		// Loop all objects, could use something more sophisticated for larger scenes but a simple loop gets us pretty far
		for (const auto& entity : Game::rootScene.entities) {
			
			glm::vec3 hitpt, normal; float depth;
			bool intersect = false;

			// Switching here is faster than a virtual function call
			switch (entity->type) {
				case Entity::Type::Sphere:
					intersect = ((Sphere*)entity.get())->Intersect(pos, dir, hitpt, normal, depth); break;
				case Entity::Type::Plane: 
					intersect = ((Plane*)entity.get())->Intersect(pos, dir, hitpt, normal, depth); break;
				case Entity::Type::RenderedMesh: 
					intersect = ((RenderedMesh*)entity.get())->Intersect(pos, dir, hitpt, normal, depth); break;
				default: break;
			}

			// Check if hit something and it's the closest hit
			if (intersect && depth < minDepth) {
				minDepth = depth;
				output = Color::FromVec(normal, 1.0f);
			}
		}

		// If we hit nothing, draw "Skybox"
		if (minDepth == std::numeric_limits<float>::max()) {
			output = Color(0x33, 0x33, 0x44, 0xff);
		}

		return output;
	}

	void TraceScene() {

		// Camera
		const glm::vec3 cameraPos = glm::vec3(3.0f, 2.0f, 5.0f);
		const glm::vec3 origin = glm::vec3(0.0f, 0.0f, 0.0f);

		// Sort scene objects
		std::ranges::sort(Game::rootScene.entities, [&](const std::unique_ptr<Entity>& a, const std::unique_ptr<Entity>& b) {
			return a->EstimatedDistanceTo(cameraPos) < b->EstimatedDistanceTo(cameraPos);
		});
		
		// Matrices
		auto view = glm::lookAt(cameraPos, origin, glm::vec3(0.0f, 1.0f, 0.0f));
		auto proj = glm::perspectiveFov(glm::radians(90.0f), (float)Game::window.width, (float)Game::window.height, 0.05f, 1000.0f);
		auto viewInv = glm::inverse(view);
		auto projInv = glm::inverse(proj);

		const bgfx::Memory* mem = bgfx::makeRef(textureBuffer, (uint32_t)textureBufferSize);

		// Modify the texture on cpu in a multithreaded function
		// @TODO: Probably can optimize this with cache locality, loop order etc but it's fast enough for now
		concurrency::parallel_for(size_t(0), size_t(textureBufferSize / 4), [&](size_t i) {
			float xcoord = ((float)(i % Game::window.width)) / Game::window.width;
			float ycoord = ((float)(i / Game::window.width)) / Game::window.height;
			
			// Create view ray from proj/view matrices
			glm::vec2 pixel = glm::vec2(xcoord, ycoord) * 2.0f - 1.0f;
			glm::vec4 px = glm::vec4(pixel, 0.0f, 1.0f);
			
			px = projInv * px;
			px.w = 0.0f;
			glm::vec3 dir = viewInv * px;
			dir = normalize(dir);
			
			textureBuffer[i] = TraceRay(cameraPos, dir);
		});

		bgfx::updateTexture2D(texture, 0, 0, 0, 0, Game::window.width, Game::window.height, mem);

		// Render a single triangle as a fullscreen pass
		if (bgfx::getAvailTransientVertexBuffer(3, vtxLayout) == 3) {

			// Grab massive 3 vertice buffer
			bgfx::TransientVertexBuffer vb;
			bgfx::allocTransientVertexBuffer(&vb, 3, vtxLayout);
			PosColorTexCoord0Vertex* vertex = (PosColorTexCoord0Vertex*)vb.data;

			// Vertices
			const Color clr(0x00, 0x00, 0x00, 0xff);
			vertex[0] = PosColorTexCoord0Vertex{ .pos = glm::vec3(0.0f, 0.0f, 0.0f),				.rgba = clr, .uv = glm::vec2(0.0f, 0.0f) };
			vertex[1] = PosColorTexCoord0Vertex{ .pos = glm::vec3(Game::window.width, 0.0f, 0.0f),	.rgba = clr, .uv = glm::vec2(2.0f, 0.0f) };
			vertex[2] = PosColorTexCoord0Vertex{ .pos = glm::vec3(0.0f, Game::window.height, 0.0f),	.rgba = clr, .uv = glm::vec2(0.0f, 2.0f) };

			// Set data and submit
			bgfx::setViewTransform(VIEW_LAYER, &view, &proj);
			bgfx::setTexture(0, u_texture, texture);
			bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
			bgfx::setVertexBuffer(0, &vb);
			bgfx::submit(VIEW_LAYER, program);
		}
	}

	~Raytracer() {
		bgfx::destroy(texture);
		bgfx::destroy(u_texture);
		delete textureBuffer;
	}
};