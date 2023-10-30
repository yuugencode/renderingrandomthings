module;

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <ppl.h> // Parallel for

export module Raytracer;

import Utils;
import Window;
import Entity;
import Scene;
import Transform;
import Shapes;
import RenderedMesh;
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

	const Window* window;

	Timer timer;

	void Create(const Window& window) {

		this->window = &window;

		vtxLayout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.end();

		// Create mutable texture
		texture = bgfx::createTexture2D(window.width, window.height, false, 1, textureFormat);
		
		bgfx::TextureInfo info;
		bgfx::calcTextureSize(info, window.width, window.height, 1, false, false, 1, textureFormat);
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

	Color TraceRay(const Scene& scene, const glm::vec3& pos, const glm::vec3& dir) {
		
		Color output;
		float minDepth = std::numeric_limits<float>::max();
		uint32_t extraData;

		Entity* intersectedObj = nullptr;

		// Loop all objects, could use something more sophisticated for larger scenes but a simple loop gets us pretty far
		for (const auto& entity : scene.entities) {
			
			bool intersect;
			float depth;
			uint32_t data;

			// Switching here is faster than a virtual function call
			switch (entity->type) {
				case Entity::Type::Sphere:
					intersect = ((Sphere*)entity.get())->Intersect(pos, dir, depth); break;
				case Entity::Type::Disk: 
					intersect = ((Disk*)entity.get())->Intersect(pos, dir, depth); break;
				case Entity::Type::Box:
					intersect = ((Box*)entity.get())->Intersect(pos, dir, depth); break;
				case Entity::Type::RenderedMesh: 
					intersect = ((RenderedMesh*)entity.get())->Intersect(pos, dir, depth, data); break;
				default: break;
			}

			// Check if hit something and it's the closest hit
			if (intersect && depth < minDepth) {
				minDepth = depth;
				extraData = data;
				intersectedObj = entity.get();
			}
		}

		if (intersectedObj != nullptr) {
			// Hit something, color it
			auto hitPt = pos + dir * minDepth;
			output = intersectedObj->GetColor(hitPt, extraData);
		}
		else { 
			// If we hit nothing, draw "Skybox"
			output = Color(0x33, 0x33, 0x44, 0xff);
		}
		
		return output;
	}

	void TraceScene(const Scene& scene) {

		timer.Start();

		const Transform& camTransform = scene.camera.transform;
		const glm::vec3 camPos = camTransform.position;
		
		// Matrices
		glm::vec3 fwd = camTransform.Forward();
		glm::vec3 up = camTransform.Up();
		auto view = glm::lookAt(camTransform.position, camTransform.position + fwd, up);
		auto proj = glm::perspectiveFov(glm::radians(scene.camera.fov), (float)window->width, (float)window->height, scene.camera.nearClip, scene.camera.farClip);
		auto viewInv = glm::inverse(view);
		auto projInv = glm::inverse(proj);

		const bgfx::Memory* mem = bgfx::makeRef(textureBuffer, (uint32_t)textureBufferSize);

		// Modify the texture on cpu in a multithreaded function
		// @TODO: Probably can optimize this with cache locality, loop order etc
		concurrency::parallel_for(size_t(0), size_t(textureBufferSize / 4), [&](size_t i) {
			float xcoord = (float)(i % window->width) / window->width;
			float ycoord = (float)(i / window->width) / window->height;
			
			// Create view ray from proj/view matrices
			glm::vec2 pixel = glm::vec2(xcoord, ycoord) * 2.0f - 1.0f;
			glm::vec4 px = glm::vec4(pixel, 0.0f, 1.0f);
			
			px = projInv * px;
			px.w = 0.0f;
			glm::vec3 dir = viewInv * px;
			dir = normalize(dir);
			
			textureBuffer[i] = TraceRay(scene, camPos, dir);
		});

		bgfx::updateTexture2D(texture, 0, 0, 0, 0, window->width, window->height, mem);

		// Render a single triangle as a fullscreen pass
		if (bgfx::getAvailTransientVertexBuffer(3, vtxLayout) == 3) {

			// Grab massive 3 vertice buffer
			bgfx::TransientVertexBuffer vb;
			bgfx::allocTransientVertexBuffer(&vb, 3, vtxLayout);
			PosColorTexCoord0Vertex* vertex = (PosColorTexCoord0Vertex*)vb.data;

			// Vertices
			const Color clr(0x00, 0x00, 0x00, 0xff);
			vertex[0] = PosColorTexCoord0Vertex{ .pos = glm::vec3(0.0f, 0.0f, 0.0f),			.rgba = clr, .uv = glm::vec2(0.0f, 0.0f) };
			vertex[1] = PosColorTexCoord0Vertex{ .pos = glm::vec3(window->width, 0.0f, 0.0f),	.rgba = clr, .uv = glm::vec2(2.0f, 0.0f) };
			vertex[2] = PosColorTexCoord0Vertex{ .pos = glm::vec3(0.0f, window->height, 0.0f),	.rgba = clr, .uv = glm::vec2(0.0f, 2.0f) };

			// Set data and submit
			bgfx::setViewTransform(VIEW_LAYER, &view, &proj);
			bgfx::setTexture(0, u_texture, texture);
			bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
			bgfx::setVertexBuffer(0, &vb);
			bgfx::submit(VIEW_LAYER, program);
		}

		timer.End();
	}

	~Raytracer() {
		bgfx::destroy(texture);
		bgfx::destroy(u_texture);
		delete textureBuffer;
	}
};