#pragma once

#include <glm/glm.hpp>

#include "Game/Scene.h"
#include "Game/Entity.h"
#include "Rendering/RayResult.h"

// Contains CPU "shaders" that calculate a BRDF or whatever unrealistic mess a given surface wants to display
class Shaders {
public:

	// Textured material
	static glm::vec4 Textured(	const Scene& scene, const RayResult& rayResult, const v2f& input, const TraceData& data);
	
	// Single color material
	static glm::vec4 PlainColor(const Scene& scene, const RayResult& rayResult, const v2f& input, const TraceData& data);

	// (Debug) XZ grid texture
	static glm::vec4 Grid(		const Scene& scene, const RayResult& rayResult, const v2f& input, const TraceData& data);
	
	// (Debug) Local space normals
	static glm::vec4 Normals(	const Scene& scene, const RayResult& rayResult, const v2f& input, const TraceData& data);
	
	// (Debug) Generic debug
	static glm::vec4 Debug(		const Scene& scene, const RayResult& rayResult, const v2f& input, const TraceData& data);

private:
	
	// Samples N closest points on a bvh for nearby lit points of given light
	static float SampleSmoothShadow(const Light& light, const v2f& input, const float& blockerDist);
	
	// Samples GI from bvh for this position
	static glm::vec4 SampleGI(const Scene& scene, const Light& light, const RayResult& rayResult, const v2f& input, const TraceData& data);
	
	// Shoots a ray towards a light returns if it hit anything before reaching it
	static bool ShadowRay(const Scene& scene, const glm::vec3& from, const glm::vec3& to, int collisionMask, int& recursionDepth, float& blockerDist);
	
	// Shoots a shadow ray and calculates a smooth shadow for given input
	static float CalculateShadow(const Scene& scene, const Light& light, const RayResult& rayResult, const v2f& input, const TraceData& data);
	
	// Loops lights and adds their contribution to diffuse and indirect terms
	static void LightLoop(const Scene& scene, const RayResult& rayResult, const v2f& input, glm::vec3& direct, glm::vec3& indirect, const TraceData& data);
};

