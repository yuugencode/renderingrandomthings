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

// @TODO: Look at 23-vectordisplay example

using namespace glm;

export class Raytracer {
	
public:

	bgfx::TextureHandle texture;
	bgfx::UniformHandle u_texture;
	bgfx::TextureFormat::Enum textureFormat = bgfx::TextureFormat::RGBA8;
	bgfx::UniformHandle u_mtx;
	bgfx::ProgramHandle program;

	Color* buffer; // Main texture data
	uint32_t bufferSize;

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
		texture = bgfx::createTexture2D(Game::window.width, Game::window.height, false, 1, textureFormat);
		
		bgfx::TextureInfo info;
		bgfx::calcTextureSize(info, Game::window.width, Game::window.height, 1, false, false, 1, textureFormat);
		
		bufferSize = info.storageSize;
		buffer = (Color*)malloc(bufferSize);

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

		bgfx::setViewClear(VIEW_LAYER, BGFX_CLEAR_COLOR, 0x000000ff);
		bgfx::setViewRect(VIEW_LAYER, 0, 0, bgfx::BackbufferRatio::Equal);
	}

	Color TraceRay(const vec3& pos, const vec3& dir) {
		
		// Fake tracing

		Color output;
		float minDepth = 99999999.9f;

		{
			// Ground plane
			const Plane plane(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, -1.0f, 0.0f));
			vec3 hitpt, normal;
			float depth;
			if (plane.Intersect(pos, dir, hitpt, normal, depth)) {
				if (depth < minDepth) {
					minDepth = depth;
					output = Color::FromFloat(0.5f, 0.5f, 0.5f, 1.0f);
				}
			}
		}
		
		{
			// Sphere
			const Sphere sphere(vec3(0.0f, 0.0f, 0.0f), 2.0f);
			vec3 hitpt, normal;
			float depth;
			if (sphere.Intersect(pos, dir, hitpt, normal, depth)) {
				if (depth < minDepth) {
					minDepth = depth;
					output = Color::FromVec(normal, 1.0f);
				}
			}
		}

		return output;
	}

	void TraceScene() {

		// Camera
		const vec3 cameraPos = vec3(0.0f, 1.0f, 5.0f);
		const vec3 origin = vec3(0.0f, 0.0f, 0.0f);

		// Matrices
		auto view = glm::lookAt(cameraPos, origin, glm::vec3(0.0f, 1.0f, 0.0f));
		auto proj = glm::perspectiveFov(glm::radians(90.0f), (float)Game::window.width, (float)Game::window.height, 0.05f, 1000.0f);
		auto viewInv = glm::inverse(view);
		auto projInv = glm::inverse(proj);

		const bgfx::Memory* mem = bgfx::makeRef(buffer, (uint32_t)bufferSize);

		// Modify the texture on cpu in a multithreaded function
		// @TODO: Probably can optimize this with cache locality, loop order etc but it's fast enough for now
		concurrency::parallel_for(size_t(0), size_t(bufferSize / 4), [&](size_t i) {
			float xcoord = ((float)(i % Game::window.width)) / Game::window.width;
			float ycoord = ((float)(i / Game::window.width)) / Game::window.height;
			
			// Create view ray from proj/view matrices
			vec2 pixel = vec2(xcoord, ycoord) * 2.0f - 1.0f;
			vec4 px = vec4(pixel, 0.0f, 1.0f);
			
			px = projInv * px;
			px.w = 0.0f;
			vec3 dir = viewInv * px;
			dir = normalize(dir);
			
			buffer[i] = TraceRay(cameraPos, dir);
			//buffer[i] = Color::FromFloat(dir.x, dir.y, dir.z, 1.0f);
		});

		bgfx::updateTexture2D(texture, 0, 0, 0, 0, Game::window.width, Game::window.height, mem);

		// Render a single triangle as a fullscreen pass
		if (bgfx::getAvailTransientVertexBuffer(3, vtxLayout) == 3) {

			bgfx::TransientVertexBuffer vb;
			bgfx::allocTransientVertexBuffer(&vb, 3, vtxLayout);
			PosColorTexCoord0Vertex* vertex = (PosColorTexCoord0Vertex*)vb.data;

			uint32_t red = 0xff000ff;
			vertex[0] = PosColorTexCoord0Vertex{ .pos = glm::vec3(0.0f, 0.0f, 0.0f),				.rgba = red, .uv = glm::vec2(0.0f, 0.0f) };
			vertex[1] = PosColorTexCoord0Vertex{ .pos = glm::vec3(Game::window.width, 0.0f, 0.0f),	.rgba = red, .uv = glm::vec2(2.0f, 0.0f) };
			vertex[2] = PosColorTexCoord0Vertex{ .pos = glm::vec3(0.0f, Game::window.height, 0.0f),	.rgba = red, .uv = glm::vec2(0.0f, 2.0f) };

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