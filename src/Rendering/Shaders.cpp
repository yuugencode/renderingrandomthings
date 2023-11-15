#include "Shaders.h"

#include "Engine/Utils.h"
#include "Game/Game.h"
#include "Game/RenderedMesh.h"
#include "Game/Shapes.h"

// Samples N closest points on a bvh for nearby lit points of given light
// Uses pre-sampled blocker distance to define an accurate smoothing upper bound
float SmoothDilatedShadows(const Scene& scene, const Light& light, const RayResult& rayResult, const v2f& input, const int& recursionDepth, const float& blockerDist) {
	using namespace glm;

	if (!light.shadowBvh.Exists()) return 0.0f;

	BvhPoint<Empty>::BvhPointData d0, d1, d2, d3;
	
	// Range limit for searches, should be higher than max expected penumbra size
	// Ideally very high, but relates to perf
	constexpr vec4 distLim = vec4(4.0f);
	vec4 dist = distLim;

	light.shadowBvh.Get4Closest(input.worldPosition, dist, d0, d1, d2, d3);
	if (dist[3] == distLim.x) return 0.0f;
	
	dist = sqrt(dist); // Bvh returns squared distances, could lerp using them for perf but produces uncanny softening
	
	float penumbraSize = Utils::InvLerpClamp(blockerDist * 0.3f, 0.0f, 3.0f); // Penumbra range
	penumbraSize *= 2.0f; // Max size of penumbra

	float distFromEdge = (dist[0] + dist[1] + dist[2] + dist[3]) * 0.25f;

	float shadow = 1.0f - Utils::InvLerpClamp(distFromEdge, 0.0f, penumbraSize);

	return shadow;
}

// Samples GI from bvh for this position
glm::vec4 SampleGI(const Light& light, const RayResult& rayResult, const v2f& input, const TraceData& data) {
	using namespace glm;

	if (!light.lightBvh.Exists()) return vec4(0.0f);

#if true
	BvhPoint<vec3>::BvhPointData d0;
	
	const float distlim = 1.0f;
	float dist = distlim;
	light.lightBvh.GetClosest(input.worldPosition, dist, d0);
	if (dist == distlim) return vec4(0.0f);

	// Remove receiving indirect light cast by self via a mask saved to the bvh during generation
	if (std::bit_cast<int>(d0.payload[1]) == rayResult.obj->id || std::bit_cast<int>(d0.payload[2]) == rayResult.obj->id) 
		return vec4(0.0f);

	dist = sqrt(dist);
	dist = 1.0f - Utils::InvLerpClamp(dist, 0.2f, distlim);

	return std::bit_cast<Color>(d0.payload[0]).ToVec4() * dist;
#endif
	
#if false // Temp while testing
	BvhPoint::BvhPointData d0, d1;
	vec2 dist = vec2(1.0f);
	light.lightBvh.Get2Closest(input.worldPosition, dist, d0, d1);
	if (dist[1] == 1.0f) return vec4(0.0f);
	
	dist = sqrt(dist);
	dist[0] = 1.0f - Utils::InvLerpClamp(dist[0], 0.5f, 1.0f);
	dist[1] = 1.0f - Utils::InvLerpClamp(dist[1], 0.5f, 1.0f);
	
	auto b = Utils::InvSegmentLerp(input.worldPosition, d0.point, d1.point);
	
	vec4 avgClr = (std::bit_cast<Color>(d0.mask).ToVec4() * (1.0f - b) +
				   std::bit_cast<Color>(d1.mask).ToVec4() * b );
	return avgClr;
#endif

#if false // Temp while testing
	BvhPoint::BvhPointData d0, d1, d2, d3;
	vec4 dist = vec4(1.0f);
	light.lightBvh.Get4Closest(input.worldPosition, dist, d0, d1, d2, d3);
	if (dist[3] == 1.0f) return vec4(0.0f);
	
	dist = sqrt(dist);
	dist[0] = 1.0f - Utils::InvLerpClamp(dist[0], 0.5f, 1.0f);
	dist[1] = 1.0f - Utils::InvLerpClamp(dist[1], 0.5f, 1.0f);
	dist[2] = 1.0f - Utils::InvLerpClamp(dist[2], 0.5f, 1.0f);
	dist[3] = 1.0f - Utils::InvLerpClamp(dist[3], 0.5f, 1.0f);
	
	auto b = Utils::InvQuadrilateral(input.worldPosition, d0.point, d1.point, d2.point, d3.point);
	
	vec4 avgClr =  (std::bit_cast<Color>(d0.mask).ToVec4() * b.x * dist[0] +
				    std::bit_cast<Color>(d1.mask).ToVec4() * b.y * dist[1] +
					std::bit_cast<Color>(d2.mask).ToVec4() * b.z * dist[2] +
					std::bit_cast<Color>(d3.mask).ToVec4() * b.w * dist[3] );
	
	return avgClr;
#endif
}

// Shoots a ray towards a light returns if it hit anything before reaching it
inline bool ShadowRay(const Scene& scene, const glm::vec3& from, const glm::vec3& to, const int& collisionMask, int& recursionDepth, float& blockerDist) {
	using namespace glm;

	// If we're passing multiple transparent things just return no shadow, mostly inf loop safeguard with textured objs
	if (recursionDepth > 5) return false;

	vec3 dir = (to - from);
	float dist = length(dir);
	dir /= dist; // Normalize
	Ray ray{ .ro = from, .rd = dir, .inv_rd = 1.0f / dir, .mask = collisionMask };
	RayResult res = Game::raytracer.RaycastScene(scene, ray);

	if (res.Hit() && res.depth < dist - 0.001f) {
		
		blockerDist += res.depth;

		// If we hit a textured object gotta check for transparency and go on recursively.. not very elegant
		// While this is very proper way of dealing with it, ideally there's an early exit here instead of sampling textures
		// Options: per object "casts shadows" flag, per-triangle extra data in bvh or an extra 1 bit buffer in the renderedmesh
		if (res.obj->HasMesh() && res.obj->HasTexture())
			if (static_cast<RenderedMesh*>(res.obj)->SampleAt(res.localPos, res.data).a == 0)
				return ShadowRay(scene, ray.ro + ray.rd * res.depth, to, res.data, ++recursionDepth, blockerDist);

		return true;
	}
	return false;
}

float CalculateShadow(const Scene& scene, const Light& light, const RayResult& rayResult, const v2f& input, const TraceData& data) {
	
	float blockerDist = 0.0f;
	int shadowRecursionCount = 0;

	// Lowpoly meshes require still bias to avoid self-shadowing artifacts
	// Could try using some smart filtering instead for example angle diff + dist requirement if self-intersect detected
	const glm::vec3 bias = input.worldNormal * 0.008f;

	// If the primary shadow ray hits a blocker -> this is in shadow -> calculate smooth shadows using light mask
	if (ShadowRay(scene, input.worldPosition + bias, light.position, rayResult.data, shadowRecursionCount, blockerDist))
		return SmoothDilatedShadows(scene, light, rayResult, input, data.recursionDepth, blockerDist);

	return 1.0f;
}


// Loops lights and adds their contribution to diffuse and indirect terms
void LightLoop(const Scene& scene, const RayResult& rayResult, const v2f& input, glm::vec3& direct, glm::vec3& indirect, const TraceData& data) {
	for (int i = 0; i < scene.lights.size(); i++) {
		const auto& light = scene.lights[i];
		if (Utils::SqrLength(input.worldPosition - light.position) > light.range * light.range)
			continue; // Skip lights out of range

		float atten, nl;
		light.CalcGenericLighting(input.worldPosition, input.worldNormal, atten, nl);

		float shadow = 1.0f;
		if (data.HasFlag(TraceData::Shadows)) shadow = CalculateShadow(scene, light, rayResult, input, data);
		
		float shading = shadow * nl;

		if (data.HasFlag(TraceData::Ambient)) shading = glm::max(shading, 0.1f); // Hardcoded ambient

		shading *= atten;

		direct += light.color * shading * light.intensity;

		if (data.HasFlag(TraceData::Indirect)) indirect += xyz(SampleGI(light, rayResult, input, data));
	}
}

// Shader that samples a texture
glm::vec4 Shaders::Textured(const Scene& scene, const RayResult& rayResult, const v2f& input, const TraceData& opts) {
	using namespace glm;

	vec4 c(vec3(0.0f), 1.0f);

	// Base color
	if (rayResult.obj->HasMesh() && rayResult.obj->HasTexture()) {

		const auto& mesh = Assets::Meshes[rayResult.obj->meshHandle];
		const auto& materialID = mesh->materials[rayResult.data / 3]; // .data = triangle index for meshes
		const auto& texID = rayResult.obj->textureHandles[materialID];

		c = Assets::Textures[texID]->SampleUVClamp(input.uv).ToVec4() * rayResult.obj->color;
	}

	// Loop lights
	vec3 direct = vec3(0.0f), indirect = vec3(0.0f);
	
	LightLoop(scene, rayResult, input, direct, indirect, opts);
	
	swizzle_xyz(c) *= direct;
	swizzle_xyz(c) += indirect;

	return c;
}

// Shader that draws a procedural grid
glm::vec4 Shaders::Grid(const Scene& scene, const RayResult& rayResult, const v2f& input, const TraceData& data) {
	using namespace glm;
	
	// Base color
	auto f = fract(rayResult.localPos * 10.0f);
	f = abs(f - 0.5f) * 2.0f;
	float a = min(f.x, min(f.y, f.z));
	float res = 0.4f + Utils::InvLerpClamp(1.0f - a, 0.9f, 1.0f);

	vec4 c = rayResult.obj->color;

	// Loop lights
	vec3 direct = vec3(0.0f), indirect = vec3(0.0f);

	LightLoop(scene, rayResult, input, direct, indirect, data);

	swizzle_xyz(c) *= direct;
	swizzle_xyz(c) += indirect;

	return c;
}

// Shader that draws normals as colors
glm::vec4 Shaders::Normals(const Scene& scene, const RayResult& rayResult, const v2f& input, const TraceData& data) {
	using namespace glm;

	// Base color
	vec4 c = vec4(abs(rayResult.faceNormal), 1.0f);

	// Loop lights
	vec3 direct = vec3(0.0f), indirect = vec3(0.0f);

	LightLoop(scene, rayResult, input, direct, indirect, data);

	swizzle_xyz(c) *= direct;
	swizzle_xyz(c) += indirect;

	return c;
}

// Shader that's just plain white
glm::vec4 Shaders::PlainColor(const Scene& scene, const RayResult& rayResult, const v2f& input, const TraceData& data) {
	using namespace glm;

	// Base color
	vec4 c(rayResult.obj->color);

	// Loop lights
	vec3 direct = vec3(0.0f), indirect = vec3(0.0f);

	LightLoop(scene, rayResult, input, direct, indirect, data);

	swizzle_xyz(c) *= direct;
	swizzle_xyz(c) += indirect;

	return c;
}

// Debug shader
glm::vec4 Shaders::Debug(const Scene& scene, const RayResult& rayResult, const v2f& input, const TraceData& data) {
	using namespace glm;

	vec4 c;
	swizzle_xyz(c) = fract(input.worldPosition * 2.0f);
	c.a = 1.0f;
	return c;
}
