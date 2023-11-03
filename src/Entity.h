#pragma once

#include <glm/glm.hpp>

#include "Utils.h"
#include "Transform.h"
#include "Bvh.h"
#include "Mesh.h"
#include "Assets.h"

// Abstract object in the scene, can be raytraced against
class Entity {
public:
	
	enum class Type { Sphere, Disk, Box, RenderedMesh };
	enum class Shader { PlainWhite, Normals, Textured, Grid };

	// Type of object this is (mesh, parametric) @TODO: SDF
	Type type;

	// What kind of shader should this use
	Shader shaderType;

	// Unique object id
	uint32_t id;

	Transform transform;
	AABB aabb;
	Bvh bvh;

	std::vector<uint32_t> textureHandles; // Indices to texture assets array
	int meshHandle = -1; // Mesh, if any
	float reflectivity = 0.0f; // How much should light bounce off this

	bool HasTexture() const { return textureHandles.size() != 0; }
	bool HasMesh() const { return meshHandle >= 0; }
	bool HasBVH() const { return bvh.Exists(); }
	bool HasAABB() const { return aabb.min != aabb.max; }

	const Mesh* GetMesh() const { return Assets::Meshes[meshHandle].get(); }

	// Fills v2f-style struct with relevant data for this shape
	virtual v2f VertexShader(const glm::vec3& pos, const glm::vec3& faceNormal, const uint32_t& data) const = 0;

protected:
	static uint32_t idCount;
};
