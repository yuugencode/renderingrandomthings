#include "Raytracer.h"

#include "Shaders.h"
#include "Shapes.h"
#include "RenderedMesh.h"

Raytracer* Raytracer::Instance = nullptr;

void Raytracer::Create(const Window& window) {

	Instance = this;
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
	textureBuffer = new Color[textureBufferSize];

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

glm::vec4 GetColor(const Scene& scene, const RayResult& rayResult, const glm::vec3& hitPoint, const int& recursionDepth) {

	// Interpolate variables depending on position in "vertex shader"
	const v2f interpolated = rayResult.obj->VertexShader(hitPoint, rayResult.localPos, rayResult.localNormal, rayResult.data);

	glm::vec4 c;

	// Shade below in "fragment shader"
	switch (rayResult.obj->shaderType) {
		case Entity::Shader::PlainWhite:c = Shaders::PlainWhite(scene, rayResult.obj, interpolated, recursionDepth); break;
		case Entity::Shader::Textured:	c = Shaders::Textured(scene, rayResult.obj, interpolated, recursionDepth); break;
		case Entity::Shader::Normals:	c = Shaders::Normals(scene, rayResult.obj, interpolated, recursionDepth); break;
		case Entity::Shader::Grid:		c = Shaders::Grid(scene, rayResult.obj, interpolated, recursionDepth); break;
		case Entity::Shader::Debug:		c = Shaders::Debug(scene, rayResult.obj, interpolated, recursionDepth); break;
		default: c = Shaders::PlainWhite(scene, rayResult.obj, interpolated, recursionDepth); break;
	}

	return c;
}

RayResult Raytracer::RaycastScene(const Scene& scene, const Ray& ray) const {

	RayResult result { 
		.localPos = glm::vec3(), 
		.localNormal = glm::vec3(0,1,0), 
		.obj = nullptr, 
		.depth = std::numeric_limits<float>::max(), 
		.data = (uint32_t)-1 
	};

	bool intersect;
	float depth;
	uint32_t data;
	glm::vec3 nrm;

	// Loop all objects, could use something more sophisticated but a simple loop gets us pretty far
	for (const auto& entity : scene.entities) {

		Ray _ray;
		
		// Inverse transform object to origin
		const auto newPos = entity->invModelMatrix * glm::vec4(ray.ro, 1.0f);
		const auto newDir = entity->invModelMatrix * glm::vec4(ray.rd, 0.0f);
		_ray = Ray{ .ro = newPos, .rd = newDir, .inv_rd = 1.0f / newDir, .mask = ray.mask };

		// Early rejection for missed rays
		if (entity->aabb.Intersect(_ray) == 0.0f) continue;

		// Check intersect (virtual function call but perf cost irrelevant compared to the entire frame)
		intersect = entity->IntersectLocal(_ray, nrm, data, depth);

		// Check if hit something and it's the closest hit
		if (intersect && depth < result.depth) {
			result.depth = depth;
			result.data = data;
			result.localPos = _ray.ro + _ray.rd * depth;
			result.localNormal = nrm;
			result.obj = entity.get();
		}
	}

	return result;
}

glm::vec4 Raytracer::TraceRay(const Scene& scene, const Ray& ray, int& recursionDepth) const {

	// Raycast
	RayResult rayResult = RaycastScene(scene, ray);

	if (rayResult.Hit()) { // Hit something, color it

		const auto hitPt = ray.ro + ray.rd * rayResult.depth;

		// Hit point color
		glm::vec4 c = GetColor(scene, rayResult, hitPt, recursionDepth);

		// Transparent or cutout
		if (c.a < 0.99f && recursionDepth < 1) {
			Ray newRay{ .ro = hitPt, .rd = ray.rd, .inv_rd = ray.inv_rd, .mask = rayResult.data };
			glm::vec4 behind = TraceRay(scene, newRay, ++recursionDepth);
			c = Utils::Lerp(c, behind, 1.0f - c.a);
		}
		
		// Indirect bounce
		if (rayResult.obj->reflectivity != 0.0f && recursionDepth < 2) {
			const auto newRo = glm::vec3(hitPt);
			const auto newRd = glm::reflect(ray.rd, rayResult.obj->transform.rotation * rayResult.localNormal);
			Ray newRay{ .ro = newRo, .rd = newRd, .inv_rd = 1.0f / newRd, .mask = rayResult.data };
			glm::vec4 reflColor = TraceRay(scene, newRay, ++recursionDepth);
			c = Utils::Lerp(c, reflColor, rayResult.obj->reflectivity);
		}

		return c;
	}
	else {
		// If we hit nothing, draw "Skybox"
		return glm::vec4(0.3f, 0.3f, 0.6f, 1.0f);
	}
}

void Raytracer::RenderScene(const Scene& scene) {

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

		int recursionDepth = 0;
		Ray ray{ .ro = camPos, .rd = dir, .inv_rd = 1.0f / dir, .mask = (uint32_t)-1 };
		glm::vec4 result = TraceRay(scene, ray, recursionDepth);
		textureBuffer[i] = Color::FromVec(result);
	});

	bgfx::updateTexture2D(texture, 0, 0, 0, 0, window->width, window->height, mem);

	// Render a single triangle as a fullscreen pass
	if (bgfx::getAvailTransientVertexBuffer(3, vtxLayout) == 3) {

		// Grab massive 3 vertice buffer
		bgfx::TransientVertexBuffer vb;
		bgfx::allocTransientVertexBuffer(&vb, 3, vtxLayout);
		PosColorTexCoord0Vertex* vertex = (PosColorTexCoord0Vertex*)vb.data;

		// @NOTE: Framebuffer uv.x flipped after swapping out of c++20 modules
		// Some #define somewhere changed probably? Causes no issues but anyway..
		
		// Vertices
		const Color clr(0x00, 0x00, 0x00, 0xff);
		vertex[0] = PosColorTexCoord0Vertex{ .pos = glm::vec3(0.0f, 0.0f, 0.0f),			.rgba = clr, .uv = glm::vec2(0.0f, 0.0f) };
		vertex[1] = PosColorTexCoord0Vertex{ .pos = glm::vec3(window->width, 0.0f, 0.0f),	.rgba = clr, .uv = glm::vec2(-2.0f, 0.0f) };
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
