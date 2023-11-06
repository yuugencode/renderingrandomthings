#include "Shaders.h"

#include "Raytracer.h"

consteval float sqr(const float& x) { return x * x; }
float sqrr(const float& x) { return x * x; }

// Samples 4 closest points on a bvh for their precalculated shadow value and does a funky quadrilateral interpolation in 3D
float SmoothShadows(const Scene& scene, const Light& light, const RayResult& rayResult, const v2f& input, const int& recursionDepth) {
	
	using namespace glm;

	if (!Raytracer::Instance->shadowBvh.Exists()) return 1.0f; // Light didn't hit any?

	BvhPoint::BvhPointData d0, d1, d2, d3;
	vec4 dists;

	Raytracer::Instance->shadowBvh.Get4Closest(input.worldPosition, dists, d0, d1, d2, d3);

	float rayDistance = (d0.shadowValue + d1.shadowValue + d2.shadowValue + d3.shadowValue) * 0.25f;
	dists = sqrt(dists);
	float avgDist = (dists[0] + dists[1] + dists[2] + dists[3]) * 0.25f;

	const float smoothingAmount = 1.0f; // How smooth are the shadows allowed to become
	const float smoothingStartDistance = 1.0f;
	const float smoothingEndDistance = 10.0f;
	float smoothingBound = Utils::InvLerpClamp(rayDistance, smoothingStartDistance, smoothingEndDistance) * smoothingAmount;

	float dilation = 0.05f * max((rayResult.depth * 0.33333f + rayDistance * 0.3f), 1.0f);
	//float dilation = 0.05f; // Low dilation = more accurate shadows, but causes artifacts where there's no data

	float shadow = 1.0f - Utils::InvLerpClamp(avgDist, dilation, dilation + 0.01f + smoothingBound);
	shadow *= shadow;

	shadow = Utils::Lerp(shadow, 0.0f, Utils::InvLerpClamp(rayDistance, 0.0f, light.range));

	return shadow;
}

// Shoots a shadow ray from point to given point, returns 0.0 if point is in shadow
float ShadowRay(const Scene& scene, const Light& light, const RayResult& rayResult, const v2f& input, const int& recursionDepth) {
	using namespace glm;

	vec3 shadowRayDir = light.position - input.worldPosition;
	float shadowRayDist = length(shadowRayDir);
	shadowRayDir /= shadowRayDist + 0.0000001f;
	
	const vec3 bias = shadowRayDir * 0.005f;
	Ray ray{ .ro = input.worldPosition + bias, .rd = shadowRayDir, .inv_rd = 1.0f / shadowRayDir, .mask = rayResult.data };
	
	RayResult result = Raytracer::Instance->RaycastScene(scene, ray);
	
	return result.depth < shadowRayDist ? 0.0f : 1.0f;
}

float CalculateShadow(const Scene& scene, const Light& light, const RayResult& rayResult, const v2f& input, const int& recursionDepth) {
	//return SmoothShadows(scene, light, rayResult, input, recursionDepth);
	return ShadowRay(scene, light, rayResult, input, recursionDepth);
	//return 1.0f;
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
		return vec4(shadow, shadow, shadow, 1.0f);
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

	// Loop lights
	//for (const auto& light : scene.lights) {
	//	float atten, nl;
	//	light.CalcGenericLighting(input.worldPosition, input.localNormal, atten, nl);
	//	float shadow = ShadowRay(scene, input.worldPosition, light.position, input.data);
	//	swizzle_xyz(c) *= nl * atten * shadow;
	//}

	return c;
}
