#include "Raytracer.h"

#include <ppl.h> // Parallel for

#include "Game/Shapes.h"
#include "Game/RenderedMesh.h"
#include "Rendering/Shaders.h"
#include "Engine/Log.h"

using namespace glm; // Math heavy file, convenience

// Construct the hardware backend for drawing 1 triangle
void Raytracer::Create(const Window& window) {

	this->window = &window;

	vtxLayout
		.begin()
		.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
		.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
		.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
		.end();

	// Create a mutable texture
	texture = bgfx::createTexture2D(window.width, window.height, false, 1, textureFormat);

	bgfx::TextureInfo info;
	bgfx::calcTextureSize(info, window.width, window.height, 1, false, false, 1, textureFormat);
	textureBufferSize = info.storageSize;
	textureBuffer = new Color[textureBufferSize];

	u_texture = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

	// Read and compile shaders
	std::filesystem::path folder = "shaders/";
	auto vShader = Utils::LoadShader(folder / "screenpass_vs.bin");
	auto fShader = Utils::LoadShader(folder / "screenpass_fs.bin");
	assert(vShader.idx != bgfx::kInvalidHandle);
	assert(fShader.idx != bgfx::kInvalidHandle);

	program = bgfx::createProgram(vShader, fShader, true);
	assert(program.idx != bgfx::kInvalidHandle);

	// Full screen pass is a single triangle so clip anything outside
	bgfx::setViewRect(VIEW_LAYER, 0, 0, bgfx::BackbufferRatio::Equal);
}

vec4 Raytracer::SampleColor(const Scene& scene, const RayResult& rayResult, const Ray& ray, TraceData& data) const {

	// Interpolate variables in "vertex shader"
	v2f interpolated = rayResult.obj->VertexShader(ray, rayResult);

	// Shade below in "fragment shader"
	vec4 c = rayResult.obj->FragmentShader(scene, rayResult, interpolated, data);

	// Reflection
	if (data.HasFlag(TraceData::Reflection)) {

		float reflectivity;
		if (rayResult.obj->type == Entity::Type::RenderedMesh)
			reflectivity = rayResult.obj->materials[rayResult.obj->GetMesh()->materialIDs[rayResult.triIndex / 3]].reflectivity;
		else
			reflectivity = rayResult.obj->materials[0].reflectivity; // Assert parametric objs have 1 material

		if (reflectivity != 0.0f && data.recursionDepth < 2) {
			vec3 newRd = reflect(ray.rd, rayResult.obj->transform.rotation * rayResult.faceNormal);
			Ray newRay{ .ro = interpolated.worldPosition, .rd = newRd, .inv_rd = 1.0f / newRd, .mask = rayResult.id };
			data.recursionDepth++;
			vec4 reflColor = TracePath(scene, newRay, data);
			c = Utils::Lerp(c, reflColor, reflectivity);
		}
	}

	return c;
}

RayResult Raytracer::RaycastScene(const Scene& scene, const Ray& ray) const {

	RayResult result{
		.localPos = vec3(),
		.faceNormal = vec3(0,1,0),
		.obj = nullptr,
		.depth = std::numeric_limits<float>::max(),
		.id = std::numeric_limits<int>::min()
	};

	bool intersect;
	float depth;
	int data;
	vec3 nrm;

	// Loop all objects, could use something more sophisticated but a simple loop gets us pretty far
	for (const auto& entity : scene.entities) {

		// Early bounding box rejection for missed rays
		if (entity->worldAABB.Intersect(ray) == 0.0f) continue;

		// Inverse transform ray to the object's space
		mat2x3 newPosDir = entity->invModelMatrix * mat2x4(
			vec4(ray.ro, 1.0f),
			vec4(ray.rd, 0.0f)
		);
		Ray localRay { .ro = newPosDir[0], .rd = newPosDir[1], .inv_rd = 1.0f / newPosDir[1], .mask = ray.mask };

		// Check intersect (virtual function call but perf cost irrelevant compared to the entire frame)
		intersect = entity->IntersectLocal(localRay, nrm, data, depth);

		// Check if hit something and if it's the closest hit
		if (intersect && depth < result.depth) {
			result.depth = depth;
			result.id = data;
			result.localPos = localRay.ro + localRay.rd * depth;
			result.faceNormal = nrm;
			result.obj = entity.get();
		}
	}

	return result;
}

vec4 Raytracer::TracePath(const Scene& scene, const Ray& ray, TraceData& data) const {

	// Raycast
	RayResult rayResult = RaycastScene(scene, ray);

	if (rayResult.Hit()) {

		// Hit something, color it

		data.cumulativeDepth += rayResult.depth;

		// Hit point color
		vec4 c = SampleColor(scene, rayResult, ray, data);

		// If we hit a transparent or cutout surface, skip it and check further
		if (data.HasFlag(TraceData::Transparent) && c.a < 0.99f && data.recursionDepth < 2) {
			vec3 hitPt = ray.ro + ray.rd * rayResult.depth;
			Ray newRay{ .ro = hitPt, .rd = ray.rd, .inv_rd = ray.inv_rd, .mask = rayResult.id };
			data.recursionDepth++;
			vec4 behind = TracePath(scene, newRay, data);
			c = Utils::Lerp(c, behind, 1.0f - c.a);
		}

		return c;
	}
	else {
		// If we hit nothing, draw "Skybox"
		if (data.HasFlag(TraceData::Skybox))
			return vec4(0.3f, 0.3f, 0.6f, 1.0f);
		else
			return vec4(0.0f, 0.0f, 0.0f, 1.0f);
	}
}

void Raytracer::SmoothShadowsPass(Scene& scene, const mat4x4& projInv, const mat4x4& viewInv) {

	// Prepass for generating light buffer for interpolating smooth shadows
	lightBufferSampleTimer.Start();

	// Clear and regenerate light buffer for each light
	for (auto& light : scene.lights) {
		light.lightBvh.Clear();
		light._lightBvhTempBuffer.clear();
	}

	// Tiling and downsampling parameters for this pass
	constexpr int sizeDiv = 4;
	constexpr int tileSize = 4;
	const int scaledWidth = window->width / sizeDiv;
	const int scaledHeight = window->height / sizeDiv;
	const int numScaledXtiles = scaledWidth / tileSize;
	const int numScaledYtiles = scaledHeight / tileSize;
	screenTempBuffer.resize(scaledWidth * scaledHeight);

	// Shoot rays from camera to find areas that are in light, these are used for smooth shadows later
	concurrency::parallel_for(0, numScaledXtiles * numScaledYtiles, [&](int tile) {

		int tileX = tile % numScaledXtiles;
		int tileY = tile / numScaledXtiles;

		for (int j = 0; j < tileSize; j++) {
			for (int i = 0; i < tileSize; i++) {
				float xcoord = ((float)tileX * tileSize + i) / (float)scaledWidth;
				float ycoord = ((float)tileY * tileSize + j) / (float)scaledHeight;
				int textureIndex = tileX * tileSize + i + ((tileY * tileSize + j) * scaledWidth);

				// Create view ray from proj/view matrices
				vec2 pixel = vec2(xcoord, ycoord) * 2.0f - 1.0f;
				vec4 px = vec4(pixel, 0.0f, 1.0f);
				px = projInv * px;
				px.w = 0.0f;
				vec3 dir = viewInv * px;
				dir = normalize(dir);

				// Raycast
				Ray ray{ .ro = scene.camera.transform.position, .rd = dir, .inv_rd = 1.0f / dir, .mask = std::numeric_limits<int>::min() };
				RayResult res = RaycastScene(scene, ray);

				if (res.Hit()) {

					const vec3 hitpt = ray.ro + ray.rd * res.depth;

					// Loop all lights and save a bitmask representing which light indices are fully lit
					int mask = 0;

					for (int j = 0; j < scene.lights.size(); j++) {
						vec3 os = scene.lights[j].position - hitpt;
						float lightDist = length(os);
						os /= lightDist;
						Ray ray2{ .ro = hitpt, .rd = os, .inv_rd = 1.0f / os, .mask = res.id };
						RayResult res2 = RaycastScene(scene, ray2);

						if (!res2.Hit() || res2.depth > lightDist - 0.001f)
							mask |= 1 << j;
					}

					// @TODO: Pack blocker dist as uint16 (ratio / max_uint16)
					// Doing shadow rays every pixel doesn't scale

					// Save all lights that are lighting this point in space
					screenTempBuffer[textureIndex] = vec4(hitpt, std::bit_cast<float>(mask));
				}
				else {
					// If we didn't hit anything just clear the index
					screenTempBuffer[textureIndex] = vec4(std::bit_cast<float>(0));
				}
			}
		}
	});

	// Add each point to each lights internal shadow buffer if the point was lit by that light
	for (const auto& val : screenTempBuffer) {
		for (int i = 0; i < scene.lights.size(); i++) {
			if ((std::bit_cast<int>(val.w) & (1 << i)) == 0) continue;
			scene.lights[i]._lightBvhTempBuffer.push_back(val);
		}
	}

	lightBufferSampleTimer.End();
	lightBufferGenTimer.Start();

	// Generate the view light buffer for each light
	concurrency::parallel_for(size_t(0), scene.lights.size(), [&](size_t i) {
		const auto& arr = scene.lights[i]._lightBvhTempBuffer;
		scene.lights[i].lightBvh.Generate(arr.data(), (int)arr.size());
	});

	lightBufferGenTimer.End();
}

void Raytracer::IndirectLightingPass(Scene& scene, const mat4x4& projInv, const mat4x4& viewInv) {

	// Shoot rays from camera to find indirect light for pts hit
	// This data could theoretically be much lower res than entire screen if blurred

	indirectSampleTimer.Start();

	for (auto& light : scene.lights) {
		light._indirectTempBuffer.resize(screenTempBuffer.size());
	}

	// Tiling and downsampling parameters for this pass
	constexpr int sizeDiv = 4;
	constexpr int tileSize = 4;
	const int scaledWidth = window->width / sizeDiv;
	const int scaledHeight = window->height / sizeDiv;
	const int numScaledXtiles = scaledWidth / tileSize;
	const int numScaledYtiles = scaledHeight / tileSize;

	// For every screenspace point, calculate the expected reflection here and save to buffer
	concurrency::parallel_for(0, numScaledXtiles * numScaledYtiles, [&](int tile) {

		int tileX = tile % numScaledXtiles;
		int tileY = tile / numScaledXtiles;

		for (int j = 0; j < tileSize; j++) {
			for (int i = 0; i < tileSize; i++) {
				float xcoord = ((float)tileX * tileSize + i) / (float)scaledWidth;
				float ycoord = ((float)tileY * tileSize + j) / (float)scaledHeight;
				int textureIndex = tileX * tileSize + i + ((tileY * tileSize + j) * scaledWidth);

				// Create view ray from proj/view matrices
				vec2 pixel = vec2(xcoord, ycoord) * 2.0f - 1.0f;
				vec4 px = vec4(pixel, 0.0f, 1.0f);
				px = projInv * px;
				px.w = 0.0f;
				vec3 dir = viewInv * px;
				dir = normalize(dir);

				// Raycast
				const Ray ray{ .ro = scene.camera.transform.position, .rd = dir, .inv_rd = 1.0f / dir, .mask = std::numeric_limits<int>::min() };
				const RayResult res = RaycastScene(scene, ray);

				if (!res.Hit()) {
					
					// If we didn't hit anything just clear the index
					for (auto& light : scene.lights)
						light._indirectTempBuffer[textureIndex] = LightbufferPt{ .pt = vec3(), .indirect {.clr = Colors::Clear, .nrm = vec3() } };
					
					continue;
				}

				// Hit something, figure out indirect for this pt
				const vec3 hitpt = ray.ro + ray.rd * res.depth;

				// Loop potential lights @TODO: Scene top level acceleration structure
				for (auto& light : scene.lights) {

					// Skip pts outside light range
					if (Utils::SqrLength(light.position - hitpt) > light.range * light.range) continue;

					vec4 indirect = vec4(0.0f);
					bool hasReflections = false;

					// Loop potential objs
					for (const auto& obj : scene.entities) {

						if (obj.get() == res.obj) continue; // Disallow self reflections @TODO: Figure out why these look weird for some models

						constexpr float maxReflDist = 4.0f;
						
						// Skip testing object if it's further than its max reflection dist
						float maxScale = max(obj->transform.scale.x, max(obj->transform.scale.y, obj->transform.scale.z));
						if (Utils::SqrLength(obj->worldAABB.ClosestPoint(hitpt) - hitpt) > maxReflDist * maxReflDist)
							continue;

						// Given raytrace hitpt + light position + object
						// Compute the theoretical reflection point the obj would cast to this point, if any
						vec3 reflectPt, worldNormal;

						// The calculations differ for each shape

						// Sphere
						if (obj->type == Entity::Type::Sphere) {

							// For spheres figuring out reflect pt + normal is trivial
							vec3 p = obj->transform.position;
							vec3 toHitpt = normalize(hitpt - p);
							vec3 toLight = normalize(light.position - p);
							reflectPt = obj->transform.position + normalize(toHitpt + toLight) * obj->transform.scale.x;
							worldNormal = normalize(reflectPt - obj->transform.position);
						}

						// Box
						else if (obj->type == Entity::Type::Box) {

							// Figure out which side hitpt is closest to in local space
							vec3 planePos;
							vec3 tfHitpt = obj->invModelMatrix * vec4(hitpt, 1.0f);
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
							mat2x3 newPosDir = obj->invModelMatrix * mat2x4(vec4(ro, 1.0f), vec4(rd, 0.0f));
							Ray rr{ .ro = newPosDir[0], .rd = newPosDir[1], .inv_rd = 1.0f / newPosDir[1], .mask = res.id };
							int data; float depth; vec3 dummy;
							float isect = obj->IntersectLocal(rr, dummy, data, depth);

							if (isect <= 0.0f) continue; // No hit

							reflectPt = ro + rd * depth;
						}

						// Mesh
						else if (obj->type == Entity::Type::RenderedMesh) {

							vec3 tfLightpos = obj->invModelMatrix * vec4(light.position, 1.0f);

							// Use reduced search range for meshes for perf
							const float distLim = (maxReflDist * 0.5f) / maxScale;
							Bvh::BvhTriangle tri;
							vec3 tfHitpt = obj->invModelMatrix * vec4(hitpt, 1.0f);

							// Sample mesh BVH for closest potentially reflecting tri
							if (!obj->bvh.GetClosestReflectiveTri(tfHitpt, tfLightpos, distLim * distLim, res.triIndex, tri, reflectPt))
								continue; // No triangles on this mesh reflect light to this pos

							reflectPt = obj->modelMatrix * vec4(reflectPt, 1.0f);
							worldNormal = normalize(obj->transform.rotation * tri.normal);
						}

						else continue; // Disk.. SDF... @TODO

						float hitptDist = length(hitpt - reflectPt);
						if (hitptDist < 0.001f) continue; // Same pos

						vec3 toHitpt = (hitpt - reflectPt) / hitptDist;
						vec3 toLight = normalize(light.position - reflectPt);

						// Trace a ray from light to the reflection point
						Ray lightRay{ .ro = light.position, .rd = -toLight, .inv_rd = -1.0f / toLight, .mask = std::numeric_limits<int>::min() };

						TraceData data = TraceData::Reflection | TraceData::Shadows;

						// Can't call TraceRay directly because we need to assume hit target == obj in case they're in shadow
						RayResult rayResult = RaycastScene(scene, lightRay);
						if (!rayResult.Hit() || rayResult.obj != obj.get()) continue;
						vec4 color = SampleColor(scene, rayResult, lightRay, data);

						//color = vec4(0.0f, 1.0f, 1.0f, 1.0f); // Debug

						// Used to fade the edges of distance pruning
						float reflectFade = 1.0f - Utils::InvLerpClamp(length(reflectPt - hitpt), 0.0f, maxReflDist);
						reflectFade *= reflectFade * reflectFade; // 1/n^3 falloff seems good?

						// Angle filter for reflection
						float ang = (1.0f - dot(toHitpt, worldNormal)) * dot(toLight, worldNormal);

						// Attenuation
						float atten = light.CalcAttenuation(length(reflectPt - hitpt) + length(reflectPt - light.position));

						float intensity = ang * atten * reflectFade * 3.0f;

						if (intensity < 1.0f / 255.0f + 0.001f) continue; // Don't add data if intensity is below 1/255

						indirect += color * intensity;

						hasReflections = true;
					}

					// Save the accumulated value
					Color clr = Color::FromVec(indirect);
					light._indirectTempBuffer[textureIndex] = LightbufferPt{ .pt = hitpt, .indirect {.clr = clr, .nrm = res.obj->transform.rotation * res.faceNormal } };
				}
			}
		}
	});

	indirectSampleTimer.End();
	indirectGenTimer.Start();

	// Populate toAdd buffer for each light and generate bvh
	concurrency::parallel_for(size_t(0), scene.lights.size(), [&](size_t i) {

		// Go over pts that had any data and add to temp buffer
		auto& light = scene.lights[i];
		light._toAddBuffer.clear();
		for (const auto& val : light._indirectTempBuffer) {
			if (val.indirect.clr.a == 0) continue;
			light._toAddBuffer.push_back(val);
		}

		// Regen BVH
		light.indirectBvh.Generate(light._toAddBuffer.data(), (int)light._toAddBuffer.size());
	});

	indirectGenTimer.End();
}

void Raytracer::MainDirectPass(Scene& scene, const mat4x4& projInv, const mat4x4& viewInv) {

	// Tiling and downsampling parameters for this pass
	constexpr int sizeDiv = 1;
	constexpr int tileSize = 4;
	const int scaledWidth = window->width / sizeDiv;
	const int scaledHeight = window->height / sizeDiv;
	const int numScaledXtiles = scaledWidth / tileSize;
	const int numScaledYtiles = scaledHeight / tileSize;

	// Main scene trace pass
	sceneTraceTimer.Start();
	concurrency::parallel_for(0, numScaledXtiles * numScaledYtiles, [&](const int tile) {
		int tileX = tile % numScaledXtiles;
		int tileY = tile / numScaledXtiles;

		for (int j = 0; j < tileSize; j++) {
			for (int i = 0; i < tileSize; i++) {
				float xcoord = (float)(tileX * tileSize + i) / (float)window->width;
				float ycoord = (float)(tileY * tileSize + j) / (float)window->height;
				int textureIndex = tileX * tileSize + i + ((tileY * tileSize + j) * window->width);

				// Create view ray from proj/view matrices
				vec2 pixel = vec2(xcoord, ycoord) * 2.0f - 1.0f;
				vec4 px = vec4(pixel, 0.0f, 1.0f);

				px = projInv * px;
				px.w = 0.0f;
				vec3 dir = viewInv * px;
				dir = normalize(dir);

				// Trace the scene
				Ray ray{ .ro = scene.camera.transform.position, .rd = dir, .inv_rd = 1.0f / dir, .mask = std::numeric_limits<int>::min() };
				TraceData data = TraceData::Default;
				vec4 result = TracePath(scene, ray, data);

				textureBuffer[textureIndex] = Color::FromVec(result);
			}
		}
	});

	sceneTraceTimer.End();
}

void Raytracer::RenderScene(Scene& scene) {

	// Matrices
	const Transform& camTransform = scene.camera.transform;
	vec3 camPos = camTransform.position;
	vec3 fwd = camTransform.Forward();
	vec3 up = camTransform.Up();
	mat4x4 view = glm::lookAt(camTransform.position, camTransform.position + fwd, up);
	mat4x4 proj = glm::perspectiveFov(radians(scene.camera.fov), (float)window->width, (float)window->height, scene.camera.nearClip, scene.camera.farClip);
	mat4x4 viewInv = inverse(view);
	mat4x4 projInv = inverse(proj);

	// Backbuffer
	const bgfx::Memory* mem = bgfx::makeRef(textureBuffer, (uint32_t)textureBufferSize);

	// Clear buffers
	for (auto& light : scene.lights) {
		light.indirectBvh.Clear();
		light.lightBvh.Clear();
	}

	// Calculate downsampled lit areas to use for smoothing shadow
	SmoothShadowsPass(scene, projInv, viewInv);

	// Calculate 1 bounce indirect lighting cast by objects
	IndirectLightingPass(scene, projInv, viewInv);

	// Draw the main screen buffer
	MainDirectPass(scene, projInv, viewInv);

	// Update gpu texture
	bgfx::updateTexture2D(texture, 0, 0, 0, 0, window->width, window->height, mem);

	// Render a single triangle as a fullscreen pass
	if (bgfx::getAvailTransientVertexBuffer(3, vtxLayout) == 3) {

		// Grab a massive 3 vertice buffer
		bgfx::TransientVertexBuffer vb;
		bgfx::allocTransientVertexBuffer(&vb, 3, vtxLayout);
		PosColorTexCoord0Vertex* vertex = (PosColorTexCoord0Vertex*)vb.data;

		// Vertices
		const Color clr(0x00, 0x00, 0x00, 0xff);
		vertex[0] = PosColorTexCoord0Vertex{ .pos = vec3(0.0f, 0.0f, 0.0f),				.rgba = clr, .uv = vec2(0.0f, 0.0f) };
		vertex[1] = PosColorTexCoord0Vertex{ .pos = vec3(window->width, 0.0f, 0.0f),	.rgba = clr, .uv = vec2(-2.0f, 0.0f) };
		vertex[2] = PosColorTexCoord0Vertex{ .pos = vec3(0.0f, window->height, 0.0f),	.rgba = clr, .uv = vec2(0.0f, 2.0f) };

		// Set data and submit
		bgfx::setViewTransform(VIEW_LAYER, &view, &proj);
		bgfx::setTexture(0, u_texture, texture);
		bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
		bgfx::setVertexBuffer(0, &vb);
		bgfx::submit(VIEW_LAYER, program);
	}
}
