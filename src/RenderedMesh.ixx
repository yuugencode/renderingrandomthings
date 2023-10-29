module;

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

export module RenderedMesh;

import <memory>;
import Entity;
import Mesh;
import Bvh;
import Timer;
import Log;

/// <summary> A mesh consisting of triangles that can be raytraced </summary>
export struct RenderedMesh : Entity {
public:

	std::shared_ptr<Mesh> mesh;

	Bvh bvh;

	RenderedMesh(const std::shared_ptr<Mesh>& mesh) {
		type = Entity::Type::RenderedMesh;
		this->mesh = mesh;

		// Construct a bvh
		bvh.Generate(mesh->vertices, mesh->triangles);
	}

	bool Intersect(const glm::vec3& ro, const glm::vec3& rd, float& depth) const {
		return bvh.Intersect(ro, rd, depth);
	}

	float EstimatedDistanceTo(const glm::vec3& pos) const {
		return glm::length(pos); // @TODO
	}

	Color GetColor(const glm::vec3& pos) const {
		return Globals::Red;
	};
};