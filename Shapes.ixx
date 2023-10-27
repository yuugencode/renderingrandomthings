module;

#include <glm/glm.hpp>

export module Shapes;

//using namespace glm; // Having this here breaks intellisense but compiles fine, sad coding

export struct Sphere {
public:

	glm::vec3 pos;
	float radius;

	Sphere(const glm::vec3& pos, const float& radius) {
		this->pos = pos;
		this->radius = radius;
	}

	// Adapted from Inigo Quilez https://iquilezles.org/articles/
	bool Intersect(const glm::vec3& ro, const glm::vec3& rd, glm::vec3& pt, glm::vec3& nrm, float& depth) const {
		glm::vec3 oc = ro - pos;
		float b = dot(oc, rd);
		float c = dot(oc, oc) - radius * radius;
		float h = b * b - c;
		if (h < 0.0f) return false;
		h = sqrt(h);
		depth = -b - h;
		pt = ro + rd * depth;
		nrm = glm::normalize(pt - pos);
		return true;
	}
};

export struct Plane {
public:

	glm::vec3 pos;
	glm::vec3 normal;

	Plane(const glm::vec3& pos, const glm::vec3& normal) {
		this->pos = pos;
		this->normal = normal;
	}

	bool Intersect(const glm::vec3& ro, const glm::vec3& rd, glm::vec3& pt, glm::vec3& nrm, float& depth) const {
		auto d = glm::dot(normal, rd);
		if (d > -0.00001f) return false;
		depth = glm::dot(pos - ro, normal) / d;
		pt = ro + rd * depth;
		nrm = normal;
		return true;
	}
};
