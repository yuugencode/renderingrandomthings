#pragma once

#include <glm/glm.hpp>

#include "Utils.h"
#include "Transform.h"
#include "Bvh.h"
#include "Mesh.h"
#include "Assets.h"

struct RayResult;

// Abstract object in the scene, can be raytraced against
class Entity {
public:
	
	enum class Type { Sphere, Disk, Box, RenderedMesh };
	enum class Shader { PlainWhite, Normals, Textured, Grid, Debug };

	// Type of object this is (mesh, parametric) @TODO: SDF
	Type type;

	// What kind of shader should this use
	Shader shaderType;

	// Unique object id
	int id = 0;

	Transform transform;
	AABB aabb;
	Bvh bvh;

	std::vector<uint32_t> textureHandles; // Indices to texture assets array
	int meshHandle = -1; // Mesh, if any
	float reflectivity = 0.0f; // How much should light bounce off this

	glm::mat4x3 invModelMatrix;

	bool HasTexture() const { return textureHandles.size() != 0; }
	bool HasMesh() const { return meshHandle >= 0; }
	bool HasBVH() const { return bvh.Exists(); }
	bool HasAABB() const { return aabb.min != aabb.max; }

	const Mesh* GetMesh() const { return Assets::Meshes[meshHandle].get(); }

	// Fills v2f-style struct with relevant data for this shape
	virtual v2f VertexShader(const glm::vec3& worldPos, const RayResult& rayResult) const = 0;

	virtual bool IntersectLocal(const Ray& ray, glm::vec3& normal, int& data, float& depth) const = 0;

protected:
	static int idCount;
};
