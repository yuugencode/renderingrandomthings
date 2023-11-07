#include "Shaders.h"

#include "Raytracer.h"

// Samples N closest points on a bvh for nearby lit points
// Uses pre-sampled blocker distance to define an accurate smoothing upper bound
float SmoothDilatedShadows(const Scene& scene, const Light& light, const RayResult& rayResult, const v2f& input, const int& recursionDepth, const float& blockerDist) {
	using namespace glm;

	if (!Raytracer::Instance->shadowBvh.Exists()) return 1.0f;

	BvhPoint::BvhPointData d0, d1, d2, d3;
	vec4 dists;
	Raytracer::Instance->shadowBvh.Get4Closest(input.worldPosition, dists, d0, d1, d2, d3);
	
	dists = sqrt(dists); // Bvh returns squared distances

	float penumbraSize = blockerDist;

	float distFromEdge = (dists[0] + dists[1] + dists[2] + dists[3]) * 0.25f;

	float shadow = 1.0f - Utils::InvLerpClamp(distFromEdge, 0.0f, penumbraSize * 0.1f);

	return shadow;
}

// Shoots a ray towards a light returns if it hit anything before reaching it
inline bool ShadowRay(const Scene& scene, const glm::vec3& from, const glm::vec3& to, const int& mask, float& blockerDist) {
	using namespace glm;
	vec3 dir = (to - from);
	float dist = length(dir);
	dir /= dist; // Normalize
	Ray ray{ .ro = from, .rd = dir, .inv_rd = 1.0f / dir, .mask = mask };
	RayResult res = Raytracer::Instance->RaycastScene(scene, ray);
	blockerDist = res.depth;
	return res.Hit() && res.depth < dist - 0.001f;
}

// Searches around the hitpoint for shadow/light transition, if found binary searches the exact point and uses that to lerp along penumbra
float TransitionRefineShadows(const Scene& scene, const Light& light, const RayResult& rayResult, const v2f& input, const int& recursionDepth, const float& blockerDist) {
	using namespace glm;

	// This method is theoretically accurate for blocky shapes, but full of artifacts for complex ones
	// Currently unused, left here in case any ideas pop up

	const float penumbraSize = blockerDist * 0.1f;

	vec3 x = normalize(cross(light.position - input.worldPosition, vec3(0, 1, 0)));
	vec3 y = cross(x, vec3(0, 1, 0));

	x *= penumbraSize; y *= penumbraSize;

	vec3 pt, closestLight;
	
	bool hit = false;
	float _blockerDist; // Unused

	// X direction search samples
	pt = input.worldPosition + x;
	if (!ShadowRay(scene, pt, light.position, rayResult.data, _blockerDist)) { closestLight = pt; hit = true; }
	pt = input.worldPosition - x;
	if (!ShadowRay(scene, pt, light.position, rayResult.data, _blockerDist)) { closestLight = pt; hit = true; }

	// Y direction search samples
	//if (!hit) {
	//	pt = input.worldPosition + y;
	//	if (!RaycastLight(scene, pt, light.position, rayResult.data, _blockerDist)) { closestLight = pt; hit = true; }
	//	pt = input.worldPosition - y;
	//	if (!RaycastLight(scene, pt, light.position, rayResult.data, _blockerDist)) { closestLight = pt; hit = true; }
	//}

	if (!hit) return 0.0f; // This pt is deep in shadow -> early exit

	// A = shadow, B = light
	vec3 a = input.worldPosition;
	vec3 b = closestLight;

	// Binary search the point where the function changes
	vec3 mid;
	int i = 0;
	do {
		mid = a * 0.5f + b * 0.5f;
		if (ShadowRay(scene, mid, light.position, rayResult.data, _blockerDist))
			a = mid; // Mid shadow -> Move A to mid
		else
			b = mid; // Mid light -> Move B to mid
	
	} while (i++ < 8); // 2^N representable steps accuracy, 8 = lossless for 8 bit color


	float distFromEdge = length(input.worldPosition - mid);

	float shadow = 1.0f - Utils::InvLerpClamp(distFromEdge, 0.0f, penumbraSize);

	return shadow;
}

float CalculateShadow(const Scene& scene, const Light& light, const RayResult& rayResult, const v2f& input, const int& recursionDepth) {
	float blockerDist;
	
	// If the primary shadow ray hits a blocker -> this is in shadow -> calculate smooth shadows
	if (ShadowRay(scene, input.worldPosition, light.position, rayResult.data, blockerDist))
		return SmoothDilatedShadows(scene, light, rayResult, input, recursionDepth, blockerDist);
	else 
		return 1.0f;
}

// Shader that samples a texture
glm::vec4 Shaders::Textured(const Scene& scene, const RayResult& rayResult, const v2f& input, const int& recursionDepth) {
	using namespace glm;

	vec4 c(1.0f);

	// Sample texture
	if (rayResult.obj->HasMesh() && rayResult.obj->HasTexture()) {

		const auto& mesh = Assets::Meshes[rayResult.obj->meshHandle];
		const auto& material = mesh->materials[rayResult.data / 3]; // .data = triangle index for meshes
		const auto& texID = rayResult.obj->textureHandles[material];

		c = Assets::Textures[texID]->SampleUVClamp(input.uv).ToVec4();
	}

	// Loop lights
	for (const auto& light : scene.lights) {
		float atten, nl;
		light.CalcGenericLighting(input.worldPosition, rayResult.obj->transform.rotation * input.localNormal, atten, nl);
		float shadow = CalculateShadow(scene, light, rayResult, input, recursionDepth);
		swizzle_xyz(c) *= nl * atten * shadow;
	}

	return c;
}

// Shader that draws a procedural grid
glm::vec4 Shaders::Grid(const Scene& scene, const RayResult& rayResult, const v2f& input, const int& recursionDepth) {
	using namespace glm;
	
	// Simple grid
	auto f = fract(rayResult.localPos * 10.0f);
	f = abs(f - 0.5f) * 2.0f;
	float a = min(f.x, min(f.y, f.z));
	float res = 0.4f + Utils::InvLerpClamp(1.0f - a, 0.9f, 1.0f);

	// Loop lights
	for (const auto& light : scene.lights) {
		float atten, nl;
		light.CalcGenericLighting(input.worldPosition, rayResult.obj->transform.rotation * rayResult.faceNormal, atten, nl);
		float shadow = CalculateShadow(scene, light, rayResult, input, recursionDepth);
		//return vec4(shadow, shadow, shadow, 1.0f);
		res *= nl * atten * shadow;
	}

	return vec4(res, res, res, 1.0f);
}

// Shader that draws normals as colors
glm::vec4 Shaders::Normals(const Scene& scene, const RayResult& rayResult, const v2f& input, const int& recursionDepth) {
	using namespace glm;

	vec4 c = vec4(abs(rayResult.faceNormal), 1.0f);

	// Loop lights
	for (const auto& light : scene.lights) {
		float atten, nl;
		light.CalcGenericLighting(input.worldPosition, rayResult.obj->transform.rotation * rayResult.faceNormal, atten, nl);
		float shadow = CalculateShadow(scene, light, rayResult, input, recursionDepth);
		swizzle_xyz(c) *= nl * atten * shadow;
	}

	return c;
}

// Shader that's just plain white
glm::vec4 Shaders::PlainWhite(const Scene& scene, const RayResult& rayResult, const v2f& input, const int& recursionDepth) {
	using namespace glm;

	vec4 c(1.0f);

	// Loop lights
	for (const auto& light : scene.lights) {
		float atten, nl;
		light.CalcGenericLighting(input.worldPosition, rayResult.obj->transform.rotation * rayResult.faceNormal, atten, nl);
		float shadow = CalculateShadow(scene, light, rayResult, input, recursionDepth);
		swizzle_xyz(c) *= nl * atten * shadow;
	}

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
