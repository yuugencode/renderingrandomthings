module;

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>

export module Transform;

import Utils;

/// <summary> Wrapper from mat4x4 and some utility functions </summary>
export struct Transform {

	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;

	glm::vec3 Forward() const { return rotation * glm::vec3(0, 0, 1); }
	glm::vec3 Up() const { return  rotation * glm::vec3(0, 1, 0); }
	glm::vec3 Right() const { return  rotation * glm::vec3(1, 0, 0); }

	void LookAtDir(const glm::vec3& dir, const glm::vec3& up) {
		rotation = glm::quatLookAt(dir, up);
	}

	glm::mat4x4 ToMatrix() {
		return glm::translate(position) * glm::mat4_cast(rotation) * glm::scale(scale);
	}
};