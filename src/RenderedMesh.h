#pragma once

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

#include "Utils.h"
#include "Entity.h"

// A mesh consisting of triangles that can be raytraced 
struct RenderedMesh : Entity {
public:

	RenderedMesh(const int& meshHandle);

	// Generates the BVH for the mesh on this obj
	void GenerateBVH();

	// Intersects a ray against the BVH of this mesh
	bool IntersectLocal(const Ray& ray, glm::vec3& normal, int& triIdx, float& depth) const;

	// Fills v2f struct with possibly interpolated data that is passed onto the "fragment shader"
	v2f VertexShader(const glm::vec3& worldPos, const RayResult& rayResult) const;
};
