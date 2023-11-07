#pragma once

#include <vector>
#include <memory>

#include "Game/Entity.h"
#include "Game/Camera.h"
#include "Rendering/Light.h"

class Scene {

public:

	// The scene is a flat structure rather than a tree for now
	std::vector<std::unique_ptr<Entity>> entities;

	// List of all lights in the scene
	std::vector<Light> lights;

	// Camera of the scene
	Camera camera;
};
