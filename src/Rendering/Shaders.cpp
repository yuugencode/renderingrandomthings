#include "Shaders.h"

#include "Engine/Utils.h"
#include "Game/Game.h"
#include "Game/RenderedMesh.h"
#include "Game/Shapes.h"

// Samples N closest points on a bvh for nearby lit points of given light
// Uses pre-sampled blocker distance to define an accurate smoothing upper bound
float SampleSmoothShadow(const Light& light, const v2f& input, const float& blockerDist) {
	using namespace glm;

	if (!light.lightBvh.Exists()) return 0.0f;

	constexpr int N = 4; // Num closest samples to average
	constexpr float distLim = 4.0f; // Range limit for searches, should be higher than max expected penumbra size
	float dist[N] { distLim, distLim, distLim, distLim };
	constexpr BvhPoint<Empty>::BvhPointData emptyData{ .point = vec3(0.0f), .payload = Empty() };
	BvhPoint<Empty>::BvhPointData data[N]{ emptyData, emptyData, emptyData, emptyData };

	light.lightBvh.GetNClosest<N>(input.worldPosition, dist, data);

	//if (dist[N - 1] == distLim) return 0.0f; // Couldn't find any pts nearby
	
	float penumbraSize = Utils::InvLerpClamp(blockerDist * 0.3f, 0.0f, 3.0f); // Penumbra range
	penumbraSize *= 2.0f; // Max size of penumbra

	// Bvh returns squared distances, could lerp using them to avoid sqrt but produces uncanny softening
	float distFromEdge = 0.0f;
	for (int i = 0; i < N; i++) distFromEdge += sqrt(dist[i]) * (1.0f / N);

	float shadow = 1.0f - Utils::InvLerpClamp(distFromEdge, 0.0f, penumbraSize);

	return shadow;
}

// Samples GI from bvh for this position
glm::vec4 SampleGI(const Light& light, const RayResult& rayResult, const v2f& input, const TraceData& data) {
	using namespace glm;

	if (!light.indirectBvh.Exists()) return vec4(0.0f);

	constexpr int N = 4;
	constexpr float distLim = 1.0f;
	constexpr BvhPoint<vec3>::BvhPointData emptyData{ .point = vec3(0.0f), .payload = vec3(0.0f) };
	BvhPoint<vec3>::BvhPointData datas[N]{ emptyData, emptyData, emptyData, emptyData };
	float dist[N]{ distLim, distLim, distLim, distLim };

	light.indirectBvh.GetNClosest<N>(input.worldPosition, dist, datas);
	
	//if (dist[N - 1] == distLim) return vec4(0.0f); // Couldn't find any pts nearby

	// Remove receiving indirect light cast by self via a mask saved to the bvh during generation
	for (int i = 0; i < N; i++) {
		if (std::bit_cast<int>(datas[i].payload[1]) == rayResult.obj->id ||
			std::bit_cast<int>(datas[i].payload[2]) == rayResult.obj->id)
			dist[i] = distLim; // Make contribution 0
	}

	// Average N nearest samples
	vec4 ret = vec4(0.0f);
	for (int i = 0; i < N; i++) {		
		dist[i] = 1.0f - Utils::InvLerpClamp(sqrt(dist[i]), 0.2f, distLim);
		ret += std::bit_cast<Color>(datas[i].payload[0]).ToVec4() * dist[i];
	}
	ret /= (float)N;
	return ret;
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
		if (res.obj->HasMesh() && res.obj->HasMaterials())
			if (static_cast<RenderedMesh*>(res.obj)->SampleAt(res.localPos, res.data).a == 0)
				return ShadowRay(scene, ray.ro + ray.rd * res.depth, to, res.data, ++recursionDepth, blockerDist);

		return true;
	}
	return false;
}

// Shoots a shadow ray and calculates a smooth shadow for given input
float CalculateShadow(const Scene& scene, const Light& light, const RayResult& rayResult, const v2f& input, const TraceData& data) {
	
	float blockerDist = 0.0f;
	int shadowRecursionCount = 0;

	// Lowpoly meshes require still bias to avoid self-shadowing artifacts
	// Could try using some smart filtering instead for example angle diff + dist requirement if self-intersect detected
	const glm::vec3 bias = input.worldNormal * 0.008f;

	// If the primary shadow ray hits a blocker -> this is in shadow -> calculate smooth shadows using light mask
	if (ShadowRay(scene, input.worldPosition + bias, light.position, rayResult.data, shadowRecursionCount, blockerDist))
		return SampleSmoothShadow(light, input, blockerDist);

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
	if (rayResult.obj->HasMesh() && rayResult.obj->HasMaterials()) {

		const auto& mesh = Assets::Meshes[rayResult.obj->meshHandle];
		const auto& materialID = mesh->materials[rayResult.data / 3]; // .data = triangle index for meshes
		const auto& material = rayResult.obj->materials[materialID];

		if (material.textureHandle != -1)
			c = Assets::Textures[material.textureHandle]->SampleUVClamp(input.uv).ToVec4() * material.color;
		else
			c = Colors::Cyan.ToVec4();
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

	vec4 c = rayResult.obj->materials[0].color;
	swizzle_xyz(c) *= res;

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
	vec4 c(rayResult.obj->materials[0].color);

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
