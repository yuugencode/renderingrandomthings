module;

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

export module Entity;

import Utils;
import Transform;
import Bvh;
import Mesh;
import Texture;
import Assets;

/// <summary> Abstract object in the scene, can be raytraced </summary>
export class Entity {
public:
	
	enum class Type { Sphere, Disk, Box, RenderedMesh };
	Type type;

	Transform transform;
	AABB aabb;
	Bvh bvh;
	std::vector<uint32_t> textureHandles;
	int meshHandle = -1;
	float reflectivity = 0.0f;

	bool HasTexture() const { return textureHandles.size() != 0; }
	bool HasMesh() const { return meshHandle >= 0; }
	bool HasBVH() const { return bvh.Exists(); }
	bool HasAABB() const { return aabb.min != aabb.max; }

	const Mesh* GetMesh() const { return Assets::Meshes[meshHandle].get(); }
};