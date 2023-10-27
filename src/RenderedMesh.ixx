module;

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

export module RenderedMesh;

import Entity;

/// <summary> A mesh consisting of triangles that can be raytraced </summary>
export class RenderedMesh : Entity {
public:

	bgfx::VertexBufferHandle vertices;
	bgfx::IndexBufferHandle indices;

	RenderedMesh() {
		type = Entity::Type::RenderedMesh;
	}

	bool Intersect(const glm::vec3& ro, const glm::vec3& rd, glm::vec3& pt, glm::vec3& nrm, float& depth) const {
		return false;
	}
};