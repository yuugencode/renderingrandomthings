#pragma once

#include <glm/glm.hpp>

#include "Game/Entity.h"

// Raycast result
struct RayResult {
	glm::vec3 localPos;
	glm::vec3 faceNormal;
	Entity* obj;
	float depth;
	int data;
	bool Hit() const { return obj != nullptr; }
};