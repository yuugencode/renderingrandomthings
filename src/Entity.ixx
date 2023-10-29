module;

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

export module Entity;

import Utils;

/// <summary> Abstract object in the scene, can be raytraced </summary>
export class Entity {
public:
	enum Type { Sphere, Disk, Box, RenderedMesh };
	Type type;
	virtual float EstimatedDistanceTo(const glm::vec3& pos) const = 0;

	virtual Color GetColor(const glm::vec3& pos) const = 0;
};