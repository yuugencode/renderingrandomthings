#pragma once

#include <glm/glm.hpp>

#include "Utils.h"
#include "Scene.h"
#include "Entity.h"

// Contains CPU "shaders" that calculate a BRDF or the unrealistic mess a surface wants to show
class Shaders {
public:
	static glm::vec4 Grid(		const Scene& scene, const Entity* obj, const v2f& input);
	static glm::vec4 Textured(	const Scene& scene, const Entity* obj, const v2f& input);
	static glm::vec4 PlainWhite(const Scene& scene, const Entity* obj, const v2f& input);
	static glm::vec4 Normals(	const Scene& scene, const Entity* obj, const v2f& input);
};
