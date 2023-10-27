module;

#include <glm/glm.hpp>

export module Shapes;

import Entity;

/// <summary> Parametrized sphere </summary>
export struct Sphere : Entity {
public:

	glm::vec3 pos;
	float radius;

	Sphere(const glm::vec3& pos, const float& radius) {
		this->pos = pos;
		this->radius = radius;
		type = Entity::Type::Sphere;
	}

	// Adapted from Inigo Quilez https://iquilezles.org/articles/
	bool Intersect(const glm::vec3& ro, const glm::vec3& rd, glm::vec3& pt, glm::vec3& nrm, float& depth) const {
		glm::vec3 oc = ro - pos;
		float b = glm::dot(oc, rd);
		float c = glm::dot(oc, oc) - radius * radius;
		float h = b * b - c;
		if (h < 0.0f) return false;
		h = sqrt(h);
		depth = -b - h;
		pt = ro + rd * depth;
		nrm = glm::normalize(pt - pos);
		return true;
	}

	float EstimatedDistanceTo(const glm::vec3& pos) const {
		return glm::max(glm::distance(pos, this->pos) - radius, 0.0f);
	}
};

/// <summary> Parametrized infinite plane </summary>
export struct Plane : Entity {
public:

	glm::vec3 pos;
	glm::vec3 normal;

	Plane(const glm::vec3& pos, const glm::vec3& normal) {
		this->pos = pos;
		this->normal = normal;
		type = Entity::Type::Plane;
	}

	bool Intersect(const glm::vec3& ro, const glm::vec3& rd, glm::vec3& pt, glm::vec3& nrm, float& depth) const {
		auto d = glm::dot(normal, rd);
		if (d > -0.00001f) return false;
		depth = glm::dot(pos - ro, normal) / d;
		pt = ro + rd * depth;
		nrm = normal;
		return true;
	}

	float EstimatedDistanceTo(const glm::vec3& pos) const {
		float b = glm::dot(normal, pos - this->pos);
		float c = glm::dot(normal, normal);
		float a = b / c;
		return glm::distance(pos, pos + normal * a);
	}
};
