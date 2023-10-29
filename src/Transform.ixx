module;

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>

export module Transform;

import Log;

/// <summary> Wrapper from mat4x4 and some utility functions </summary>
export struct Transform {
public:

	glm::mat4x4 matrix;

	Transform() {}
	Transform(const glm::mat4x4& mat) {
		matrix = mat;
	}

	glm::vec3 Position() const	{ return  glm::vec3(matrix[3]); }
	glm::vec3 Forward() const	{ return -glm::normalize(glm::vec3(matrix[2])); } // sign dependent on left/right handedness etc
	glm::vec3 Up() const		{ return  glm::normalize(glm::vec3(matrix[1])); }
	glm::vec3 Right() const		{ return  glm::normalize(glm::vec3(matrix[0])); }

	void SetPosition(const glm::vec3& pos) {
		matrix[3][0] = pos.x;
		matrix[3][1] = pos.y;
		matrix[3][2] = pos.z;
	}
	void Translate(const glm::vec3& offset) {
		SetPosition(Position() + offset);
	}

	void LocalTranslate(const glm::vec3& offset) {
		SetPosition(Position() + Right() * offset.x + Up() * offset.y + Forward() * offset.z);
	}

	void LookAtDir(const glm::vec3 dir, const glm::vec3& up) {
		auto newRot = glm::quatLookAt(dir, up);
		auto rot = glm::quat_cast(matrix);
		auto os = glm::inverse(rot) * newRot;
		matrix = matrix * glm::mat4x4(glm::normalize(os));
	}

	explicit operator glm::mat4x4() const { return matrix; }
};