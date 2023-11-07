#pragma once

#include <glm/glm.hpp>

#include "Game/Scene.h"
#include "Game/Entity.h"
#include "Rendering/RayResult.h"

// Contains CPU "shaders" that calculate a BRDF or whatever unrealistic mess a given surface wants to display
class Shaders {
public:
	static glm::vec4 Grid(		const Scene& scene, const RayResult& rayResult, const v2f& input, const int& recursionDepth);
	static glm::vec4 Textured(	const Scene& scene, const RayResult& rayResult, const v2f& input, const int& recursionDepth);
	static glm::vec4 PlainWhite(const Scene& scene, const RayResult& rayResult, const v2f& input, const int& recursionDepth);
	static glm::vec4 Normals(	const Scene& scene, const RayResult& rayResult, const v2f& input, const int& recursionDepth);
	static glm::vec4 Debug(		const Scene& scene, const RayResult& rayResult, const v2f& input, const int& recursionDepth);
};
