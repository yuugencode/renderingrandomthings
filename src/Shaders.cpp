module;

#include <glm/glm.hpp>

module Shaders;

import Utils;
import Scene;
import Raytracer;
import Game;
import Assets;
import Mesh;
import Texture;

using namespace glm; // doesn't leak outside the module

// Shoots a shadow ray towards given point
float ShadowRay(const Scene& scene, const vec3& from, const vec3& to, const uint32_t& mask) {
	vec3 shadowRayDir = to - from;
	float shadowRayDist = length(shadowRayDir);
	shadowRayDir /= shadowRayDist + 0.0000001f;
	Ray ray{ .ro = from, .rd = shadowRayDir, .inv_rd = 1.0f / shadowRayDir, .mask = mask };
	RayResult result = Raytracer::Instance->TraceScene(scene, ray);
	return result.depth < shadowRayDist ? 0.5f : 1.0f;
}

// Shader that draws a procedural grid
vec4 Shaders::Grid(const Scene& scene, const Entity* obj, const v2f& input) {
	
	// Simple grid
	auto f = fract(input.position * 2.0f);
	f = abs(f - 0.5f) * 2.0f;
	float a = min(f.x, min(f.y, f.z));
	float res = 0.3f + Utils::InvLerpClamp(1.0f - a, 0.95f, 1.0f);

	// Loop lights
	for (const auto& light : scene.lights) {
		float atten, nl;
		light.CalcGenericLighting(input.position, input.normal, atten, nl);
		float shadow = ShadowRay(scene, input.position, light.position, input.data);
		res *= nl * atten * shadow;
	}

	return vec4(res, res, res, 1.0f);
}

// Shader that draws normals as colors
vec4 Shaders::Normals(const Scene& scene, const Entity* obj, const v2f& input) {

	vec4 c = vec4(abs(input.normal), 1.0f);

	// Loop lights
	for (const auto& light : scene.lights) {
		float atten, nl;
		light.CalcGenericLighting(input.position, input.normal, atten, nl);
		float shadow = ShadowRay(scene, input.position, light.position, input.data);
		swizzle_xyz(c) *= nl * atten * shadow;
	}

	return c;
}

// Shader that samples a texture
vec4 Shaders::Textured(const Scene& scene, const Entity* obj, const v2f& input) {

	vec4 c(1.0f);

	// Textured mesh
	if (obj->HasMesh() && obj->HasTexture()) {

		const auto& mesh = Assets::Meshes[obj->meshHandle];
		const auto& material = mesh->materials[input.data / 3]; // .data = triangle index for meshes
		const auto& texID = obj->textureHandles[material];

		c = Assets::Textures[texID]->SampleUVClamp(input.uv).ToVec4();
	}

	// Loop lights
	for (const auto& light : scene.lights) {

		float atten, nl;
		light.CalcGenericLighting(input.position, input.normal, atten, nl);

		float shadow = ShadowRay(scene, input.position, light.position, input.data);
		shadow = max(shadow, obj->reflectivity);

		swizzle_xyz(c) *= nl * atten * shadow;
	}

	return c;
}

// Shader that's just plain white
vec4 Shaders::PlainWhite(const Scene& scene, const Entity* obj, const v2f& input) {

	vec4 c(1.0f);

	// Loop lights
	for (const auto& light : scene.lights) {
		float atten, nl;
		light.CalcGenericLighting(input.position, input.normal, atten, nl);
		float shadow = ShadowRay(scene, input.position, light.position, input.data);
		swizzle_xyz(c) *= nl * atten * shadow;
	}

	return c;
}
