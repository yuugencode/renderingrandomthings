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
	}

	void GenerateBVH(const glm::vec3& camDir) {
		// Construct a bvh
		Timer t(1);
		t.Start();
		bvh.Generate(mesh->vertices, mesh->triangles); 
		t.End();
		Log::Line("bvh gen", Log::FormatFloat((float)t.GetAveragedTime() * 1000.0f), "ms");
	}

	bool Intersect(const glm::vec3& ro, const glm::vec3& rd, float& depth) const {
		int minIndex; // Unused here, should be passed up to give info on triangle we hit
		return bvh.Intersect(ro, rd, depth, minIndex);
	}

	float EstimatedDistanceTo(const glm::vec3& pos) const {
		return glm::length(pos); // @TODO
	}

	Color GetColor(const glm::vec3& pos) const {
		return Globals::Red;
	};
};