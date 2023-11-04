#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// Spatial info struct
struct Transform {

	glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::quat rotation = glm::quat();
	glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);

	glm::vec3 Forward() const;
	glm::vec3 Up() const;
	glm::vec3 Right() const;

	void LookAtDir(const glm::vec3& dir, const glm::vec3& up);

	// Returns this transform as a model matrix
	glm::mat4x4 ToMatrix();
};
