#pragma once

#include <vector>
#include <memory>

#include "Game/Entity.h"
#include "Game/Camera.h"
#include "Rendering/Light.h"

class Entity; // Entity cross-references this so forward declare

class Scene {

public:

	// The scene is a flat structure rather than a tree for now
	std::vector<std::unique_ptr<Entity>> entities;

	// List of all lights in the scene
	std::vector<Light> lights;

	// Camera of the scene
	Camera camera;

	// Highly variable function that reads and/or generates a bunch of whatever test models are currently used
	void ReadAndAddTestObjects();

	// Trivial getter, null if doesn't exist
	Entity* GetObjectByName(const std::string& name) const;
};
