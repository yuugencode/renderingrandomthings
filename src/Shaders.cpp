#include "Shaders.h"

#include "Raytracer.h"

// Samples 4 closest points on a bvh for their precalculated shadow value and does a funky quadrilateral interpolation in 3D
float SmoothShadows(const Scene& scene, const glm::vec3& from, const glm::vec3& to, const uint32_t& mask, const int& recursionDepth) {
	
	using namespace glm;

	BvhPoint::BvhPointData d0, d1, d2, d3;
	Raytracer::Instance->shadowBvh.Get4Closest(from, d0, d1, d2, d3);

	vec4 b = Utils::InvQuadrilateral(from, d0.point, d1.point, d2.point, d3.point);

	float shadow = d0.shadowValue * b.x + 
				   d1.shadowValue * b.y + 
		           d2.shadowValue * b.z + 
		           d3.shadowValue * b.w;

	return shadow;
}

// Shoots a shadow ray from point to given point, returns 0.0 if point is in shadow
float ShadowRay(const Scene& scene, const glm::vec3& from, const glm::vec3& to, const int& mask, const int& recursionDepth) {
	using namespace glm;

	vec3 shadowRayDir = to - from;
	float shadowRayDist = length(shadowRayDir);
	shadowRayDir /= shadowRayDist + 0.0000001f;
	
	const vec3 bias = shadowRayDir * 0.005f;
	Ray ray{ .ro = from + bias, .rd = shadowRayDir, .inv_rd = 1.0f / shadowRayDir, .mask = mask };
	
	RayResult result = Raytracer::Instance->RaycastScene(scene, ray);
	
	return result.depth < shadowRayDist ? 0.0f : 1.0f;
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
		//float shadow = ShadowRay(scene, input.worldPosition, light.position, rayResult.data, recursionDepth);
		float shadow = SmoothShadows(scene, input.worldPosition, light.position, rayResult.data, recursionDepth);
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
		//float shadow = ShadowRay(scene, input.worldPosition, light.position, input.data, recursionDepth);
		float shadow = SmoothShadows(scene, input.worldPosition, light.position, rayResult.data, recursionDepth);
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
		//float shadow = ShadowRay(scene, input.worldPosition, light.position, rayResult.data, recursionDepth);
		float shadow = SmoothShadows(scene, input.worldPosition, light.position, rayResult.data, recursionDepth);
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
		//float shadow = ShadowRay(scene, input.worldPosition, light.position, rayResult.data, recursionDepth);
		float shadow = SmoothShadows(scene, input.worldPosition, light.position, rayResult.data, recursionDepth);
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
