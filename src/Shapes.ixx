module;

#include <glm/glm.hpp>

export module Shapes;

import Entity;
import Utils;

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
	bool Intersect(const glm::vec3& ro, const glm::vec3& rd, float& depth) const {
		glm::vec3 oc = ro - pos;
		float b = glm::dot(oc, rd);
		if (b > 0.0f) return false;
		float c = glm::dot(oc, oc) - radius * radius;
		float h = b * b - c;
		if (h < 0.0f) return false;
		h = sqrt(h);
		depth = -b - h;
		return true;
	}

	float EstimatedDistanceTo(const glm::vec3& pos) const {
		return glm::max(glm::distance(pos, this->pos) - radius, 0.0f);
	}

	Color GetColor(const glm::vec3& pos) const {
		auto nrm = glm::normalize(pos - this->pos);
		return Color::FromVec(nrm, 1.0f);
	};
};

/// <summary> Parametrized infinite plane </summary>
export struct Disk : Entity {
public:

	glm::vec3 pos;
	glm::vec3 normal;
	float radius;

	Disk(const glm::vec3& pos, const glm::vec3& normal, const float& radius) {
		this->pos = pos;
		this->normal = normal;
		this->radius = radius;
		type = Entity::Type::Disk;
	}

	bool Intersect(const glm::vec3& ro, const glm::vec3& rd, float& depth) const {
		glm::vec3 o = ro - pos;
		depth = -glm::dot(normal, o) / glm::dot(rd, normal);
		if (depth < 0.0f) return false;
		glm::vec3  q = o + rd * depth;
		if (dot(q, q) < radius * radius) return true;
		return false;
	}

	float EstimatedDistanceTo(const glm::vec3& pos) const {
		auto os = pos - this->pos;
		os = Utils::Project(os, normal);
		return glm::length(os);
	}

	Color GetColor(const glm::vec3& pos) const {

		// Simple grid
		auto f = glm::fract(pos * 2.0f);
		f = glm::abs(f - 0.5f) * 2.0f;
		float a = glm::min(f.x, glm::min(f.y, f.z));
		return Color::FromFloat(0.3f + Utils::InvLerpClamp(1.0f - a, 0.95f, 1.0f)); //Color(0xaa, 0xaa, 0xbb, 0xff);
	};
};

/// <summary> A box </summary>
export struct Box : Entity {
public:

	AABB aabb;

	Box(const glm::vec3& pos, const float& radius) {
		aabb = AABB(pos - radius, pos + radius);
		type = Entity::Type::Box;
	}

	bool Intersect(const glm::vec3& ro, const glm::vec3& rd, float& depth) const {
		auto res = aabb.Intersect(ro, rd);
		if (res > 0.0f) {
			depth = res;
			return true;
		}
		return false;
	}

	float EstimatedDistanceTo(const glm::vec3& pos) const {
		return glm::max(glm::distance(pos, aabb.Center()) - aabb.Size().x, 0.0f);
	}

	Color GetColor(const glm::vec3& pos) const {
	
		auto nrm = pos - aabb.Center();
		
		if (glm::abs(nrm.x) > glm::abs(nrm.y) && glm::abs(nrm.x) > glm::abs(nrm.z))
			nrm = nrm.x < 0.0f ? glm::vec3(1, 1, 0) : glm::vec3(1, 0, 0);
		else if (glm::abs(nrm.y) > glm::abs(nrm.z))
			nrm = nrm.y < 0.0f ? glm::vec3(0, 1, 1) : glm::vec3(0, 1, 0);
		else
			nrm = nrm.z < 0.0f ? glm::vec3(1, 0, 1) : glm::vec3(0, 0, 1);

		return Color::FromVec(nrm, 1.0f);
	};
};
