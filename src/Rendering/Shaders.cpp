#include "Shaders.h"

#include "Engine/Utils.h"
#include "Game/Game.h"
#include "Game/RenderedMesh.h"

// Samples N closest points on a bvh for nearby lit points of given light
// Uses pre-sampled blocker distance to define an accurate smoothing upper bound
float SmoothDilatedShadows(const Scene& scene, const Light& light, const RayResult& rayResult, const v2f& input, const int& recursionDepth, const float& blockerDist) {
	using namespace glm;

	if (!light.shadowBvh.Exists()) return 0.0f;

	BvhPoint::BvhPointData d0, d1, d2, d3;
	
	// Range limit for searches, should be higher than max expected penumbra size
	// Ideally very high, but relates to perf
	vec4 dist = vec4(2.0f);

	light.shadowBvh.Get4Closest(input.worldPosition, dist, d0, d1, d2, d3);
	dist = sqrt(dist); // Bvh returns squared distances, could lerp using them for perf but produces uncanny softening
	
	float penumbraSize = blockerDist * 0.3f; // example values 0.1 for buffer div 2, 0.3 for buffer div 4

	float distFromEdge = (dist[0] + dist[1] + dist[2] + dist[3]) * 0.25f;

	float shadow = 1.0f - Utils::InvLerpClamp(distFromEdge, 0.0f, penumbraSize * penumbraSize);

	return shadow;
}

// Shoots a ray towards a light returns if it hit anything before reaching it
inline bool ShadowRay(const Scene& scene, const glm::vec3& from, const glm::vec3& to, const int& collisionMask, int& recursionDepth, float& blockerDist) {
	using namespace glm;

	// If we're passing multiple transparent things just return no shadow, mostly inf loop safeguard with textured objs
	if (recursionDepth > 5) return 1.0f;

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
			if (static_cast<RenderedMesh*>(res.obj)->IsTransparentAt(res.data, res.localPos))
				return ShadowRay(scene, ray.ro + ray.rd * res.depth, to, res.data, ++recursionDepth, blockerDist);

		return true;
	}
	return false;
}

float CalculateShadow(const Scene& scene, const Light& light, const RayResult& rayResult, const v2f& input, const int& recursionDepth) {
	
	float blockerDist = 0.0f;
	int shadowRecursionCount = 0;

	// Lowpoly meshes require still bias to avoid self-shadowing artifacts
	// Could try using some smart filtering instead for example angle diff + dist requirement if self-intersect detected
	const glm::vec3 bias = input.worldNormal * 0.008f;

	// If the primary shadow ray hits a blocker -> this is in shadow -> calculate smooth shadows using light mask
	if (ShadowRay(scene, input.worldPosition + bias, light.position, rayResult.data, shadowRecursionCount, blockerDist))
		return SmoothDilatedShadows(scene, light, rayResult, input, recursionDepth, blockerDist);
	else 
		return 1.0f;
}

// Shader that samples a texture
glm::vec4 Shaders::Textured(const Scene& scene, const RayResult& rayResult, const v2f& input, const int& recursionDepth) {
	using namespace glm;

	vec4 c(vec3(0.0f), 1.0f);

	// Base color
	if (rayResult.obj->HasMesh() && rayResult.obj->HasTexture()) {

		const auto& mesh = Assets::Meshes[rayResult.obj->meshHandle];
		const auto& materialID = mesh->materials[rayResult.data / 3]; // .data = triangle index for meshes
		const auto& texID = rayResult.obj->textureHandles[materialID];

		c = Assets::Textures[texID]->SampleUVClamp(input.uv).ToVec4();
	}

	// Loop lights
	vec3 lighting = vec3(0.0f);
	
	for (int i = 0; i < scene.lights.size(); i++) {
		const auto& light = scene.lights[i];
		if (Utils::SqrLength(input.worldPosition - light.position) > light.range * light.range)
			continue; // Skip lights out of range

		float atten, nl;
		light.CalcGenericLighting(input.worldPosition, input.worldNormal, atten, nl);
		float shadow = CalculateShadow(scene, light, rayResult, input, recursionDepth);
		lighting += light.color * nl * atten * shadow * light.intensity;
	}

	swizzle_xyz(c) *= lighting;

	return c;
}

// Shader that draws a procedural grid
glm::vec4 Shaders::Grid(const Scene& scene, const RayResult& rayResult, const v2f& input, const int& recursionDepth) {
	using namespace glm;
	
	// Base color
	auto f = fract(rayResult.localPos * 10.0f);
	f = abs(f - 0.5f) * 2.0f;
	float a = min(f.x, min(f.y, f.z));
	float res = 0.4f + Utils::InvLerpClamp(1.0f - a, 0.9f, 1.0f);

	vec4 c = vec4(vec3(res), 1.0f);

	// Loop lights
	vec3 lighting = vec3(0.0f);

	for (int i = 0; i < scene.lights.size(); i++) {
		const auto& light = scene.lights[i];
		if (Utils::SqrLength(input.worldPosition - light.position) > light.range * light.range)
			continue; // Skip lights out of range

		float atten, nl;
		light.CalcGenericLighting(input.worldPosition, input.worldNormal, atten, nl);
		float shadow = CalculateShadow(scene, light, rayResult, input, recursionDepth);
		lighting += light.color * nl * atten * shadow * light.intensity;
	}

	swizzle_xyz(c) *= lighting;

	return c;
}

// Shader that draws normals as colors
glm::vec4 Shaders::Normals(const Scene& scene, const RayResult& rayResult, const v2f& input, const int& recursionDepth) {
	using namespace glm;

	// Base color
	vec4 c = vec4(abs(rayResult.faceNormal), 1.0f);

	// Loop lights
	vec3 lighting = vec3(0.0f);
	
	for (int i = 0; i < scene.lights.size(); i++) {
		const auto& light = scene.lights[i];
		if (Utils::SqrLength(input.worldPosition - light.position) > light.range * light.range)
			continue; // Skip lights out of range

		float atten, nl;
		light.CalcGenericLighting(input.worldPosition, input.worldNormal, atten, nl);
		float shadow = CalculateShadow(scene, light, rayResult, input, recursionDepth);
		lighting += light.color * nl * atten * shadow * light.intensity;
	}

	swizzle_xyz(c) *= lighting;

	return c;
}

// Shader that's just plain white
glm::vec4 Shaders::PlainWhite(const Scene& scene, const RayResult& rayResult, const v2f& input, const int& recursionDepth) {
	using namespace glm;

	// Base color
	vec4 c(1.0f);

	// Loop lights
	vec3 lighting = vec3(0.0f);

	for (int i = 0; i < scene.lights.size(); i++) {
		const auto& light = scene.lights[i];
		if (Utils::SqrLength(input.worldPosition - light.position) > light.range * light.range)
			continue; // Skip lights out of range

		float atten, nl;
		light.CalcGenericLighting(input.worldPosition, input.worldNormal, atten, nl);
		float shadow = CalculateShadow(scene, light, rayResult, input, recursionDepth);
		lighting += light.color * nl * atten * shadow * light.intensity;
	}

	swizzle_xyz(c) *= lighting;

	return c;
}

// Debug shader
glm::vec4 Shaders::Debug(const Scene& scene, const RayResult& rayResult, const v2f& input, const int& recursionDepth) {
	using namespace glm;

	vec4 c;
	swizzle_xyz(c) = fract(input.worldPosition * 2.0f);
	c.a = 1.0f;
	return c;
}
