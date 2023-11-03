#include "Transform.h"

#include <glm/gtx/transform.hpp>

glm::vec3 Transform::Forward() const { return rotation * glm::vec3(0, 0, 1); }
glm::vec3 Transform::Up() const { return  rotation * glm::vec3(0, 1, 0); }
glm::vec3 Transform::Right() const { return  rotation * glm::vec3(1, 0, 0); }

void Transform::LookAtDir(const glm::vec3& dir, const glm::vec3& up) {
	rotation = glm::quatLookAt(dir, up);
}

glm::mat4x4 Transform::ToMatrix() {
	return glm::translate(position) * glm::mat4_cast(rotation) * glm::scale(scale);
}
