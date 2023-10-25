module;

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

export module Raytracer;

import Game;
import <memory>;
import <filesystem>;

// @TODO: Look at 23-vectordisplay example

export class Raytracer {

public:

	bgfx::TextureHandle texture;
	bgfx::UniformHandle u_texture;
	bgfx::UniformHandle u_mtx;
	bgfx::ProgramHandle program;

	const bgfx::Memory* buffer;
	const bgfx::ViewId VIEW_LAYER = 0;

	bgfx::VertexLayout vtxLayout;

	// Vertex layout for full screen pass
	struct PosColorTexCoord0Vertex {
		glm::vec3 pos;
		uint32_t rgba;
		glm::vec2 uv;
	};

	Raytracer() {

		vtxLayout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.end();

		// Create mutable texture
		texture = bgfx::createTexture2D(Game::window.width, Game::window.height, false, 1, bgfx::TextureFormat::RGB8);
		buffer = bgfx::alloc(Game::window.width * Game::window.height * 4); // Needs manual freeing?

		// Fill a test texture with screen UV values
		for (uint32_t i = 0; i < buffer->size / 3; i++) {
			float xcoord = ((float)(i % Game::window.width)) / Game::window.width;
			float ycoord = ((float)(i / Game::window.width)) / Game::window.height;

			auto clr = Globals::Black;
			clr = clr.Lerp(Globals::Red, xcoord);
			clr = clr.Lerp(Globals::Green, ycoord);

			buffer->data[i * 3 + 0] = clr.r;
			buffer->data[i * 3 + 1] = clr.g;
			buffer->data[i * 3 + 2] = clr.b;
		}

		bgfx::updateTexture2D(texture, 0, 0, 0, 0, Game::window.width, Game::window.height, buffer);

		u_texture = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);
		u_mtx = bgfx::createUniform("u_mtx", bgfx::UniformType::Mat4);

		// @TODO: Create bvh from scene objs

		// Read and compile shaders
		const std::filesystem::path folder = "shaders/";
		auto vShader = Utils::LoadShader(folder / "screenpass_vs.bin");
		auto fShader = Utils::LoadShader(folder / "screenpass_fs.bin");
			
		assert(vShader.idx != bgfx::kInvalidHandle);
		assert(fShader.idx != bgfx::kInvalidHandle);

		program = bgfx::createProgram(vShader, fShader, true);
		assert(program.idx != bgfx::kInvalidHandle);

		bgfx::setViewRect(VIEW_LAYER, 0, 0, bgfx::BackbufferRatio::Equal);
	}

	void TraceScene() {
		
		// Render a single triangle as a fullscreen pass
		if (bgfx::getAvailTransientVertexBuffer(3, vtxLayout) == 3) {

			bgfx::TransientVertexBuffer vb;
			bgfx::allocTransientVertexBuffer(&vb, 3, vtxLayout);
			PosColorTexCoord0Vertex* vertex = (PosColorTexCoord0Vertex*)vb.data;

			uint32_t red = 0xff000ff;
			vertex[0] = PosColorTexCoord0Vertex{ .pos = glm::vec3(0.0f, 0.0f, 0.0f),				.rgba = red, .uv = glm::vec2(0.0f, 0.0f) };
			vertex[1] = PosColorTexCoord0Vertex{ .pos = glm::vec3(Game::window.width, 0.0f, 0.0f),	.rgba = red, .uv = glm::vec2(2.0f, 0.0f) };
			vertex[2] = PosColorTexCoord0Vertex{ .pos = glm::vec3(0.0f, Game::window.height, 0.0f),	.rgba = red, .uv = glm::vec2(0.0f, 2.0f) };

			auto view = glm::lookAtLH(
				glm::vec3(0.0f, 0.0f, 1.0f), 
				glm::vec3(0.0f, 0.0f, 0.0f), 
				glm::vec3(0.0f, 1.0f, 0.0f));

			auto proj = glm::perspectiveFovLH(glm::radians(90.0f), (float)Game::window.width, (float)Game::window.height, 0.05f, 1000.0f);
			bgfx::setViewTransform(VIEW_LAYER, &view, &proj);

			bgfx::setTexture(0, u_texture, texture);
			bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
			//bgfx::setUniform(u_mtx, )
			bgfx::setVertexBuffer(0, &vb);
			bgfx::submit(VIEW_LAYER, program);
		}
	}

	~Raytracer() {
		bgfx::destroy(texture);
		bgfx::destroy(u_texture);
	}
};