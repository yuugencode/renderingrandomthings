#pragma once

#include <glm/glm.hpp>

#include "Engine/Common.h"
#include "Engine/Utils.h"
#include "Engine/Bvh.h"
#include "Engine/Material.h"
#include "Engine/Mesh.h"
#include "Engine/Assets.h"
#include "Game/Transform.h"

struct RayResult; // RayResults contain an entity pointer so have to declare it here

// Abstract object in the scene that can be raytraced against
class Entity {
public:
	enum class Type { Sphere, Disk, Box, RenderedMesh };

	// Type of object this is (mesh, parametric) @TODO: SDF
	Type type;

	// Name of this object
	std::string name;

	// What kind of shader should this use
	Shader shaderType;

	// Unique object id, used for masking rays for procedural objects
	int id = 0;

	Transform transform;
	AABB aabb;
	AABB worldAABB;
	Bvh bvh;

	// "Material" properties
	int meshHandle = -1; // Mesh, if any

	glm::mat4x4 modelMatrix;
	glm::mat4x3 invModelMatrix;

	std::vector<Material> materials;
	bool HasMesh() const { return meshHandle >= 0; }

	const Mesh* GetMesh() const { return Assets::Meshes[meshHandle].get(); }

	// Fills v2f-style struct with relevant data for this shape
	virtual v2f VertexShader(const Ray& ray, const RayResult& rayResult) const = 0;

	// Intersects a ray against this object in the object's local space
	virtual bool IntersectLocal(const Ray& ray, glm::vec3& normal, int& data, float& depth) const = 0;

protected:
	static int idCount;
};
