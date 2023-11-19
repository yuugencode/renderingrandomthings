#pragma once

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

#include "Engine/Utils.h"
#include "Game/Entity.h"

// A mesh consisting of triangles that can be raytraced 
struct RenderedMesh : Entity {
public:

	RenderedMesh(const int& meshHandle);

	// Generates the BVH for the mesh on this obj
	void GenerateBVH();

	// Intersects a ray against the BVH of this mesh
	bool IntersectLocal(const Ray& ray, glm::vec3& normal, int& triIdx, float& depth) const;

	// Samples the mesh texture for transparency at a given triangle index + pos
	Color SampleAt(const glm::vec3& pos, const int& data) const;

	// Samples a given triangle and barycentric UV for its raw color
	Color SampleTriangle(const int& triIndex, const glm::vec3& barycentric) const;

	// Fills v2f struct with possibly interpolated data that is passed onto the "fragment shader"
	v2f VertexShader(const Ray& ray, const RayResult& rayResult) const;
};
