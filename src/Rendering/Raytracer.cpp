#include "Raytracer.h"

#include <ppl.h> // Parallel for

#include "Game/Shapes.h"
#include "Game/RenderedMesh.h"
#include "Rendering/Shaders.h"
#include "Engine/Log.h"

void Raytracer::Create(const Window& window) {

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

	screenTempBuffer.resize((window.width * window.height) / screenTempBufferDiv);
	indirectBuffer.resize(screenTempBuffer.size());

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

glm::vec4 Raytracer::GetColor(const Scene& scene, const RayResult& rayResult, const Ray& ray, TraceData& data) const {

	// Interpolate variables in "vertex shader"
	const v2f interpolated = rayResult.obj->VertexShader(ray, rayResult);

	glm::vec4 c;

	// Shade below in "fragment shader"
	switch (rayResult.obj->shaderType) {
		case Entity::Shader::PlainWhite:c = Shaders::PlainColor(scene, rayResult, interpolated, data); break;
		case Entity::Shader::Textured:	c = Shaders::Textured(scene, rayResult, interpolated, data); break;
		case Entity::Shader::Normals:	c = Shaders::Normals(scene, rayResult, interpolated, data); break;
		case Entity::Shader::Grid:		c = Shaders::Grid(scene, rayResult, interpolated, data); break;
		case Entity::Shader::Debug:		c = Shaders::Debug(scene, rayResult, interpolated, data); break;
		default: c = Shaders::PlainColor(scene, rayResult, interpolated, data); break;
	}

	// Reflection
	if (data.HasFlag(TraceData::Reflection) && rayResult.obj->reflectivity != 0.0f && data.recursionDepth < 2) {
		const auto newRd = glm::reflect(ray.rd, rayResult.obj->transform.rotation * rayResult.faceNormal);
		Ray newRay{ .ro = interpolated.worldPosition, .rd = newRd, .inv_rd = 1.0f / newRd, .mask = rayResult.data };
		data.recursionDepth++;
		glm::vec4 reflColor = TraceRay(scene, newRay, data);
		c = Utils::Lerp(c, reflColor, rayResult.obj->reflectivity);
	}

	return c;
}

RayResult Raytracer::RaycastScene(const Scene& scene, const Ray& ray) const {

	RayResult result{
		.localPos = glm::vec3(),
		.faceNormal = glm::vec3(0,1,0),
		.obj = nullptr,
		.depth = std::numeric_limits<float>::max(),
		.data = std::numeric_limits<int>::min()
	};

	bool intersect;
	float depth;
	int data;
	glm::vec3 nrm;

	// Loop all objects, could use something more sophisticated but a simple loop gets us pretty far
	for (const auto& entity : scene.entities) {

		Ray _ray;
		
		// Inverse transform object to origin
		const auto newPosDir = entity->invModelMatrix * glm::mat2x4(
			glm::vec4(ray.ro, 1.0f),
			glm::vec4(ray.rd, 0.0f)
		);
		_ray = Ray{ .ro = newPosDir[0], .rd = newPosDir[1], .inv_rd = 1.0f / newPosDir[1], .mask = ray.mask };

		// Early bounding box rejection for missed rays
		if (entity->aabb.Intersect(_ray) == 0.0f) continue;

		// Check intersect (virtual function call but perf cost irrelevant compared to the entire frame)
		intersect = entity->IntersectLocal(_ray, nrm, data, depth);

		// Check if hit something and if it's the closest hit
		if (intersect && depth < result.depth) {
			result.depth = depth;
			result.data = data;
			result.localPos = _ray.ro + _ray.rd * depth;
			result.faceNormal = nrm;
			result.obj = entity.get();
		}
	}

	return result;
}

glm::vec4 Raytracer::TraceRay(const Scene& scene, const Ray& ray, TraceData& data) const {

	// Raycast
	RayResult rayResult = RaycastScene(scene, ray);

	if (rayResult.Hit()) { // Hit something, color it

		data.cumulativeDepth += rayResult.depth;

		// Hit point color
		glm::vec4 c = GetColor(scene, rayResult, ray, data);

		// If we hit a transparent or cutout surface, skip it and check further
		if (c.a < 0.99f && data.recursionDepth < 2) {
			const auto hitPt = ray.ro + ray.rd * rayResult.depth;
			Ray newRay{ .ro = hitPt, .rd = ray.rd, .inv_rd = ray.inv_rd, .mask = rayResult.data };
			data.recursionDepth++;
			glm::vec4 behind = TraceRay(scene, newRay, data);
			c = Utils::Lerp(c, behind, 1.0f - c.a);
		}

		return c;
	}
	else {
		// If we hit nothing, draw "Skybox"
		if (data.HasFlag(TraceData::Skybox))
			return glm::vec4(0.3f, 0.3f, 0.6f, 1.0f);
		else
			return glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	}
}

void Raytracer::RenderScene(Scene& scene) {

	using namespace glm;

	// Matrices
	const Transform& camTransform = scene.camera.transform;
	const glm::vec3 camPos = camTransform.position;
	glm::vec3 fwd = camTransform.Forward();
	glm::vec3 up = camTransform.Up();
	auto view = glm::lookAt(camTransform.position, camTransform.position + fwd, up);
	auto proj = glm::perspectiveFov(glm::radians(scene.camera.fov), (float)window->width, (float)window->height, scene.camera.nearClip, scene.camera.farClip);
	auto viewInv = glm::inverse(view);
	auto projInv = glm::inverse(proj);

	// Backbuffer
	const bgfx::Memory* mem = bgfx::makeRef(textureBuffer, (uint32_t)textureBufferSize);

	const auto scaledWidth = window->width / screenTempBufferDiv;
	const auto scaledHeight = window->height / screenTempBufferDiv;

	// Prepass for generating shadow buffer for interpolating smooth shadows
	shadowTimerSample.Start();

	// Clear and regenerate shadow buffer for each light
	for (auto& light : scene.lights) {
		light.shadowBvh.Clear();
		light._shadowTempBuffer.clear();
	}

#if true // Shadow buffer rays shot from the camera
	
	// Shoot rays from camera to find areas that are in light, these are used for smooth shadows later
	concurrency::parallel_for(size_t(0), screenTempBuffer.size(), [&](size_t i) {
		float xcoord = (float)(i % scaledWidth) / scaledWidth;
		float ycoord = (float)(i / scaledWidth) / scaledHeight;
	
		// Create view ray from proj/view matrices
		glm::vec2 pixel = glm::vec2(xcoord, ycoord) * 2.0f - 1.0f;
		glm::vec4 px = glm::vec4(pixel, 0.0f, 1.0f);
		px = projInv * px;
		px.w = 0.0f;
		glm::vec3 dir = viewInv * px;
		dir = normalize(dir);
		
		// Raycast
		const Ray ray{ .ro = camPos, .rd = dir, .inv_rd = 1.0f / dir, .mask = std::numeric_limits<int>::min() };
		auto res = RaycastScene(scene, ray);
		
		if (res.Hit()) {
			
			const auto hitpt = ray.ro + ray.rd * res.depth;

			// Loop all lights and save a bitmask representing which light indices are fully lit
			int mask = 0;

			for (int j = 0; j < scene.lights.size(); j++) {
				auto os = scene.lights[j].position - hitpt;
				auto lightDist = glm::length(os);
				os /= lightDist;
				const Ray ray2{ .ro = hitpt, .rd = os, .inv_rd = 1.0f / os, .mask = res.data };
				const auto res2 = RaycastScene(scene, ray2);

				if (!res2.Hit() || res2.depth > lightDist - 0.001f)
					mask |= 1 << j;
			}

			// Save all lights that are lighting this point in space
			screenTempBuffer[i] = glm::vec4(hitpt, std::bit_cast<float>(mask));
		}
		else {
			// If we didn't hit anything just clear the index
			screenTempBuffer[i] = glm::vec4(std::bit_cast<float>(0));
		}
	});
	
	// Add each point to each lights internal shadow buffer if the point was lit by that light
	for (const auto& val : screenTempBuffer) {
		for (int i = 0; i < scene.lights.size(); i++) {
			if ((std::bit_cast<int>(val.w) & (1 << i)) == 0) continue;
			scene.lights[i]._shadowTempBuffer.push_back(val);
		}
	}

#endif

#if false // Shadow rays shot from lights

	// Limit the count of samples per light, these are extra data mostly useful for reflections backups
	const size_t samplesPerLight = scene.lights.size() > 0 ? glm::min(bvhBuffer.size() / scene.lights.size(), (size_t)(2 << 12)) : 0;

	for (int i = 0; i < scene.lights.size(); i++) {
		const auto& light = scene.lights[i];

		const glm::vec2 rngBase = Utils::Hash23(light.position);

		concurrency::parallel_for(size_t(0), samplesPerLight, [&](size_t j) {
			
			glm::vec2 rngSeed = rngBase * (float)(j + 1);

			// Generate random direction in 3D
			auto rng = Utils::Hash22(rngSeed) * Globals::PI;
			rng.y *= 2.0f;
			auto s = glm::sin(rng);
			auto c = glm::cos(rng);
			auto dir = glm::normalize(glm::vec3(s.x * c.y, s.x * s.y, c.x));

			const Ray ray{ .ro = light.position, .rd = dir, .inv_rd = 1.0f / dir, .mask = std::numeric_limits<int>::min() };
			auto res = RaycastScene(scene, ray);

			if (res.Hit()) 
				bvhBuffer[i * samplesPerLight + j] = glm::vec4(ray.ro + ray.rd * res.depth, std::bit_cast<float>(1 << i));
			else 
				bvhBuffer[i * samplesPerLight + j] = glm::vec4(std::bit_cast<float>(0));
		});
	}

	// Add each point to each lights internal shadow buffer if the point was lit by that light
	for (int i = 0; i < samplesPerLight * scene.lights.size(); i++) {
		const auto& val = bvhBuffer[i];
		for (int i = 0; i < scene.lights.size(); i++) {
			if ((std::bit_cast<int>(val.w) & (1 << i)) == 0) continue;
			scene.lights[i]._shadowBuffer.push_back(val);
		}
	}

#endif
	shadowTimerSample.End();
	shadowTimerGen.Start();

	// Generate the view light buffer for each light
	concurrency::parallel_for(size_t(0), scene.lights.size(), [&](size_t i) {
		const auto& arr = scene.lights[i]._shadowTempBuffer;
		scene.lights[i].shadowBvh.Generate(arr.data(), (int)arr.size());
	});

	shadowTimerGen.End();

#if true

	// Shoot rays from camera to find indirect light for pts hit
	// This data can be much lower res than entire screen

	for (auto& light : scene.lights) {
		light._indirectTempBuffer.resize(screenTempBuffer.size());
		light.lightBvh.Clear();
	}

	constexpr TraceData optss = TraceData::Reflection;

	// For every screenspace point, calculate the expected reflection here and save to buffer
	concurrency::parallel_for(size_t(0), screenTempBuffer.size(), [&](size_t i) {
		float xcoord = (float)(i % scaledWidth) / scaledWidth;
		float ycoord = (float)(i / scaledWidth) / scaledHeight;

		// Create view ray from proj/view matrices
		glm::vec2 pixel = glm::vec2(xcoord, ycoord) * 2.0f - 1.0f;
		glm::vec4 px = glm::vec4(pixel, 0.0f, 1.0f);
		px = projInv * px;
		px.w = 0.0f;
		glm::vec3 dir = viewInv * px;
		dir = normalize(dir);

		// Raycast
		const Ray ray{ .ro = camPos, .rd = dir, .inv_rd = 1.0f / dir, .mask = std::numeric_limits<int>::min() };
		auto res = RaycastScene(scene, ray);

		if (res.Hit()) {

			const auto hitpt = ray.ro + ray.rd * res.depth;

			for (auto& light : scene.lights) {

				vec4 indirect = vec4(0.0f);
				bool hasReflections = false;
				ivec2 ids = ivec2(0, 0); // Support masking 2 lights per point out

				for (const auto& obj : scene.entities) {
					
					if (obj.get() == res.obj) continue; // Disallow self reflections

					// Inverse transform object to origin and test dist, skip if too far
					glm::vec3 tfHitpt = obj->invModelMatrix * glm::vec4(hitpt, 1.0f);
					float maxScale = max(obj->transform.scale.x, max(obj->transform.scale.y, obj->transform.scale.z));
					float llen = glm::length((obj->aabb.ClosestPoint(tfHitpt) - tfHitpt) * obj->transform.scale * 0.5f);
					if (llen > maxScale) continue; // If further than max scale axis -> can't reflect here -> skip

					glm::vec3 reflectPt, worldNormal;
					int reflectMask;

					if (obj->type == Entity::Type::Sphere) {

						// For spheres figuring out reflect pt + normal is trivial
						auto p = obj->transform.position;
						auto toHitpt = glm::normalize(hitpt - p);
						auto toLight = glm::normalize(light.position - p);
						reflectPt = obj->transform.position + glm::normalize(toHitpt + toLight) * obj->transform.scale.x;
						worldNormal = glm::normalize(reflectPt - obj->transform.position);
						reflectMask = obj->id;
					}
					else if (obj->type == Entity::Type::Box) {
						
						// Figure out which side hitpt is closest to in local space
						vec3 planePos;
						if (abs(tfHitpt.x) > abs(tfHitpt.y) && abs(tfHitpt.x) > abs(tfHitpt.z))
							worldNormal = vec3(1.0f, 0.0f, 0.0f) * sign(tfHitpt.x);
						else if (abs(tfHitpt.y) > abs(tfHitpt.z))
							worldNormal = vec3(0.0f, 1.0f, 0.0f) * sign(tfHitpt.y);
						else
							worldNormal = vec3(0.0f, 0.0f, 1.0f) * sign(tfHitpt.z);
						
						// Transform local normal to world space along with scale and compute mid point on that plane
						worldNormal = obj->transform.rotation * (worldNormal * obj->transform.scale); // World normal scaled
						planePos = obj->transform.position + worldNormal;
						worldNormal = normalize(worldNormal); // Renormalize

						// Reflect a point behind this plane -> Intersection test with this fake point is the reflection pos
						vec3 reflPt = planePos - reflect((planePos - hitpt), worldNormal);

						// Do the intersection test in local space
						vec3 ro = light.position;
						vec3 rd = normalize(reflPt - light.position);
						const auto newPosDir = obj->invModelMatrix * glm::mat2x4(vec4(ro, 1.0f), vec4(rd, 0.0f));
						Ray rr{ .ro = newPosDir[0], .rd = newPosDir[1], .inv_rd = 1.0f / newPosDir[1], .mask = res.data };
						int data; float depth;
						float isect = obj->IntersectLocal(rr, reflPt, data, depth);
						
						if (isect <= 0.0f) continue; // No hit

						reflectPt = ro + rd * depth;
						reflectMask = obj->id;
					}
					else {
						continue;
					}

					vec3 toHitpt = glm::normalize(hitpt - reflectPt);
					vec3 toLight = glm::normalize(light.position - reflectPt);

					// Trace a ray from light to the reflection point
					Ray lightRay{ .ro = light.position, .rd = -toLight, .inv_rd = -1.0f / toLight, .mask = std::numeric_limits<int>::min() };
					
					TraceData data = TraceData::Reflection | TraceData::Shadows;
					
					// Can't call TraceRay directly because we need to assume hit target == obj
					RayResult rayResult = RaycastScene(scene, lightRay);
					if (!rayResult.Hit() || rayResult.obj != obj.get()) continue;
					glm::vec4 color = GetColor(scene, rayResult, lightRay, data);

					//color = vec4(0.0f, 1.0f, 1.0f, 1.0f); // Debug

					// Used to fade the edges of distance pruning
					float reflectFade = 1.0f - Utils::InvLerpClamp(glm::length(reflectPt - hitpt), 0.0f, 2.0f);

					// Angle filter for reflection
					float ang = (1.0f - dot(toHitpt, worldNormal)) * dot(toLight, worldNormal);

					// Attenuation
					float atten = light.CalcAttenuation(glm::length(reflectPt - hitpt) + glm::length(reflectPt - light.position));

					indirect += color * ang * atten * reflectFade;

					if (ids[0] == 0) ids[1] = obj->id;
					else ids[0] = obj->id;

					hasReflections = true;
				}
				
				// Ensure we leave empty data around the edges to fix interpolation issues when sampling
				if (hasReflections) indirect.a = max(indirect.a, 0.01f);

				auto clr = Color::FromVec(indirect);
				
				light._indirectTempBuffer[i] = Light::BufferPt{ .pt = hitpt, .clr = clr, .ids = ids };
			}
		}
		else {
			// If we didn't hit anything just clear the index
			for (auto& light : scene.lights)
				light._indirectTempBuffer[i] = Light::BufferPt{ .pt = vec3(), .clr = Colors::Clear, .ids = ivec2(0,0) };
		}
	});

	// Populate toAdd buffer for each light and generate bvh
	concurrency::parallel_for(size_t(0), scene.lights.size(), [&](size_t i) {
		auto& light = scene.lights[i];
		light._toAddBuffer.clear();
		for (const auto& val : light._indirectTempBuffer) {
			if (val.clr.a == 0) continue;
			light._toAddBuffer.push_back(val);
		}
	});

	concurrency::parallel_for(size_t(0), scene.lights.size(), [&](size_t i) {
		auto& light = scene.lights[i];
		light.lightBvh.Generate(light._toAddBuffer.data(), (int)light._toAddBuffer.size());
	});

#endif

	traceTimer.Start();

	// Modify the texture on cpu in a multithreaded function
	// @TODO: Probably can optimize this with cache locality, loop order etc
	concurrency::parallel_for(size_t(0), size_t(textureBufferSize / sizeof(Color)), [&](size_t i) {
		float xcoord = (float)(i % window->width) / window->width;
		float ycoord = (float)(i / window->width) / window->height;

		// Create view ray from proj/view matrices
		glm::vec2 pixel = glm::vec2(xcoord, ycoord) * 2.0f - 1.0f;
		glm::vec4 px = glm::vec4(pixel, 0.0f, 1.0f);

		px = projInv * px;
		px.w = 0.0f;
		glm::vec3 dir = viewInv * px;
		dir = normalize(dir);

		const Ray ray{ .ro = camPos, .rd = dir, .inv_rd = 1.0f / dir, .mask = std::numeric_limits<int>::min() };
		TraceData data = TraceData::Default;
		glm::vec4 result = TraceRay(scene, ray, data);

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

	traceTimer.End();
}
