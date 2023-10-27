module;

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

export module RenderedMesh;

import <memory>;
import Entity;
import Mesh;

/// <summary> A mesh consisting of triangles that can be raytraced </summary>
export struct RenderedMesh : Entity {
public:

	std::shared_ptr<Mesh> mesh;

	RenderedMesh(const std::shared_ptr<Mesh>& mesh) {
		type = Entity::Type::RenderedMesh;
		this->mesh = mesh;

		// Construct a bvh
	}

	bool Intersect(const glm::vec3& ro, const glm::vec3& rd, glm::vec3& pt, glm::vec3& nrm, float& depth) const {
		// @TODO
		return false;
	}

	float EstimatedDistanceTo(const glm::vec3& pos) const {
		// @TODO
		return 0.0f;
	}
};