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
	int textureHandle;
	int meshHandle;

	bool HasTexture() const { return textureHandle >= 0; }
	bool HasMesh() const { return meshHandle >= 0; }
	bool HasBVH() const { return bvh.Exists(); }
	bool HasAABB() const { return aabb.min != aabb.max; }

	const Mesh* GetMesh() const { return Assets::Meshes[meshHandle].get(); }
	const Texture* GetTexture() const { return Assets::Textures[textureHandle].get(); }
};