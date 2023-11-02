module;

#include <glm/glm.hpp>

export module Shaders;

import Utils;
import Scene;
import Entity;

export class Shaders {
public:
	static glm::vec4 Grid(		const Scene& scene, const Entity* obj, const v2f& input);
	static glm::vec4 Textured(	const Scene& scene, const Entity* obj, const v2f& input);
	static glm::vec4 PlainWhite(const Scene& scene, const Entity* obj, const v2f& input);
	static glm::vec4 Normals(	const Scene& scene, const Entity* obj, const v2f& input);
};