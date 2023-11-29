#include "Shaders.h"

#include "Engine/Utils.h"
#include "Game/Game.h"
#include "Game/RenderedMesh.h"
#include "Game/Shapes.h"

using namespace glm; // Math heavy file, convenience

// Shader that samples a texture
vec4 Shaders::Textured(const Scene& scene, const RayResult& rayResult, const v2f& input, const TraceData& opts) {

	vec4 c(vec3(0.0f), 1.0f);

	// Base color
	if (rayResult.obj->HasMesh()) {

		const auto& mesh = Assets::Meshes[rayResult.obj->meshHandle];
		const int& materialID = mesh->materialIDs[rayResult.triIndex / 3];
		const Material& material = rayResult.obj->materials[materialID];

		if (material.HasTexture())
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
vec4 Shaders::Grid(const Scene& scene, const RayResult& rayResult, const v2f& input, const TraceData& data) {
	
	// Grid pattern
	vec3 f = fract(rayResult.localPos * 10.0f);
	f = abs(f - 0.5f) * 2.0f;
	float a = min(f.x, min(f.y, f.z));
	float gridPattern = clamp(1.0f - (Utils::InvLerpClamp(1.0f - a, 0.95f, 1.0f)), 0.5f, 1.0f);

	// Base color
	vec4 c = rayResult.obj->materials[0].color;
	swizzle_xyz(c) *= gridPattern;

	// Add texture with procedural uvs if any set
	if (rayResult.obj->materials[0].HasTexture()) {
		vec2 fake_uv = fract(vec2(input.worldPosition.x, input.worldPosition.z));
		c *= Assets::Textures[rayResult.obj->materials[0].textureHandle]->SampleUVClamp(fake_uv).ToVec4();
	}

	// Loop lights
	vec3 direct = vec3(0.0f), indirect = vec3(0.0f);

	LightLoop(scene, rayResult, input, direct, indirect, data);

	swizzle_xyz(c) *= direct;
	swizzle_xyz(c) += indirect;

	return c;
}

// Shader that draws normals as colors
vec4 Shaders::Normals(const Scene& scene, const RayResult& rayResult, const v2f& input, const TraceData& data) {

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
vec4 Shaders::PlainColor(const Scene& scene, const RayResult& rayResult, const v2f& input, const TraceData& data) {

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
vec4 Shaders::Debug(const Scene& scene, const RayResult& rayResult, const v2f& input, const TraceData& data) {
	vec4 c = vec4(1.0f);
	swizzle_xyz(c) = fract(input.worldPosition * 2.0f);
	return c;
}

// Loops lights and adds their contribution to diffuse and indirect terms
void Shaders::LightLoop(const Scene& scene, const RayResult& rayResult, const v2f& input, vec3& direct, vec3& indirect, const TraceData& data) {

	for (int i = 0; i < scene.lights.size(); i++) {
		const auto& light = scene.lights[i];
		if (Utils::SqrLength(input.worldPosition - light.position) > light.range * light.range)
			continue; // Skip lights out of range

		float atten, nl;
		light.CalcGenericLighting(input.worldPosition, input.worldNormal, atten, nl);

		float shadow = 1.0f;
		if (data.HasFlag(TraceData::Shadows)) shadow = CalculateShadow(scene, light, rayResult, input, data);

		float shading = shadow * nl;

		if (data.HasFlag(TraceData::Ambient)) shading = max(shading, 0.1f); // Hardcoded ambient

		shading *= atten;

		direct += light.color * shading * light.intensity;

		if (data.HasFlag(TraceData::Indirect)) indirect += xyz(SampleGI(scene, light, rayResult, input, data));
	}
}

// Shoots a shadow ray and calculates a smooth shadow for given input
float Shaders::CalculateShadow(const Scene& scene, const Light& light, const RayResult& rayResult, const v2f& input, const TraceData& data) {

	float blockerDist = 0.0f;
	int shadowRecursionCount = 0;

	// Lowpoly meshes still require bias to avoid self-shadowing artifacts
	// Could try using some smart filtering instead for example angle diff + dist requirement if self-intersect detected
	const vec3 bias = input.worldNormal * 0.008f;

	// If the primary shadow ray hits a blocker -> this is in shadow -> calculate smooth shadows using light mask
	if (ShadowRay(scene, input.worldPosition + bias, light.position, rayResult.id, shadowRecursionCount, blockerDist))
		return SampleSmoothShadow(light, input, blockerDist);

	return 1.0f;
}

// Shoots a ray towards a light returns if it hit anything before reaching it
inline bool Shaders::ShadowRay(const Scene& scene, const vec3& from, const vec3& to, const int& collisionMask, int& recursionDepth, float& blockerDist) {

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
		if (res.obj->type == Entity::Type::RenderedMesh)
			if (static_cast<RenderedMesh*>(res.obj)->SampleAt(res.localPos, res.id).a == 0)
				return ShadowRay(scene, ray.ro + ray.rd * res.depth, to, res.triIndex, ++recursionDepth, blockerDist);

		return true;
	}
	return false;
}

// Samples N closest points on a bvh for nearby lit points of given light
// Uses pre-sampled blocker distance to define an accurate smoothing upper bound
float Shaders::SampleSmoothShadow(const Light& light, const v2f& input, const float& blockerDist) {

	if (!light.lightBvh.Exists()) return 0.0f;

	constexpr int N = 4; // Num closest samples to average
	constexpr float distLim = 4.0f; // Range limit for searches, should be higher than max expected penumbra size
	float dist[N]{ distLim, distLim, distLim, distLim };
	constexpr BvhPoint<Empty>::BvhPointData emptyData{ .point = vec3(0.0f), .payload = Empty() };
	BvhPoint<Empty>::BvhPointData data[N]{ emptyData, emptyData, emptyData, emptyData };

	light.lightBvh.GetNClosest<N>(input.worldPosition, dist, data);

	//if (dist[N - 1] == distLim) return 0.0f; // Couldn't find any pts nearby

	float penumbraSize = Utils::InvLerpClamp(blockerDist * 0.3f, 0.0f, 3.0f); // Penumbra range
	penumbraSize *= 2.2f; // Max size of penumbra, finicky artifacts if too high

	// Bvh returns squared distances, could lerp using them to avoid sqrt but produces uncanny softening
	float distFromEdge = 0.0f;
	for (int i = 0; i < N; i++) distFromEdge += sqrt(dist[i]) * (1.0f / N);

	float shadow = 1.0f - Utils::InvLerpClamp(distFromEdge, 0.0f, penumbraSize);

	return shadow;
}

// Samples GI from bvh for this position
vec4 Shaders::SampleGI(const Scene& scene, const Light& light, const RayResult& rayResult, const v2f& input, const TraceData& data) {

	if (!light.indirectBvh.Exists()) return vec4(0.0f);

	constexpr int N = 4;
	constexpr float distLim = 1.0f;
	constexpr BvhPoint<LightbufferPayload>::BvhPointData emptyData{ .point = vec3(0.0f), .payload {.clr = Colors::Clear, .nrm = vec3(0.0f) } };
	BvhPoint<LightbufferPayload>::BvhPointData datas[N]{ emptyData, emptyData, emptyData, emptyData };
	float dist[N]{ distLim, distLim, distLim, distLim };

	light.indirectBvh.GetNClosest<N>(input.worldPosition, dist, datas);

	// Increase sample smoothing range based on view angle + view distance
	const float angOffset = dot(input.worldNormal, normalize(scene.camera.transform.position - input.worldPosition));
	const float depthOffset = (data.cumulativeDepth * 0.008f) / max(angOffset, 0.001f);

	// Average N nearest samples
	vec4 ret = vec4(0.0f);
	for (int i = 0; i < N; i++) {
		dist[i] = 1.0f - Utils::InvLerpClamp(sqrt(dist[i]), depthOffset + 0.005f, depthOffset + 0.01f);
		// Surface receiver normal has to be similar to bvh points saved normal in order to receive indirect
		float anglePenalty = max(0.0f, dot(input.worldNormal, datas[i].payload.nrm));
		ret += std::bit_cast<Color>(datas[i].payload.clr).ToVec4() * dist[i] * anglePenalty;
	}
	ret /= (float)N;
	return ret;
}