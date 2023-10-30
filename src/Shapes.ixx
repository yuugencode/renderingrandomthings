module;

#include <glm/glm.hpp>

export module Shapes;

import Entity;
import Utils;

/// <summary> Parametrized sphere </summary>
export struct Sphere : Entity {
public:

	const glm::vec3& GetPos() const { return transform.position; }

	Sphere(const glm::vec3& pos, const float& radius) {
		aabb = AABB(pos, radius);
		transform.position = pos;
		transform.scale = glm::vec3(radius);
		type = Entity::Type::Sphere;
	}

	bool Intersect(const glm::vec3& ro, const glm::vec3& rd, float& depth, uint32_t& data) const { return Intersect(ro, rd, depth); }
	bool Intersect(const glm::vec3& ro, const glm::vec3& rd, float& depth) const {
		// Adapted from Inigo Quilez https://iquilezles.org/articles/
		glm::vec3 oc = ro - transform.position;
		float b = glm::dot(oc, rd);
		if (b > 0.0f) return false;
		float c = glm::dot(oc, oc) - transform.scale.x * transform.scale.x;
		float h = b * b - c;
		if (h < 0.0f) return false;
		h = sqrt(h);
		depth = -b - h;
		return true;
	}

	float EstimatedDistanceTo(const glm::vec3& pos) const {
		return glm::max(glm::distance(pos, transform.position) - transform.scale.x, 0.0f);
	}

	Color GetColor(const glm::vec3& pos, const uint32_t& extraData) const {
		auto nrm = glm::normalize(pos - transform.position);
		return Color::FromVec(nrm, 1.0f);
	};
};

/// <summary> Parametrized infinite plane </summary>
export struct Disk : Entity {
public:

	// Disk is fine with 1 normal as orientation, save quat calculations by packing it to rotation xyz
	const glm::vec3& Normal() const { return *((glm::vec3*)&transform.rotation); }

	Disk(const glm::vec3& pos, const glm::vec3& normal, const float& radius) {
		aabb = AABB(pos, radius);
		transform.position = pos;
		memcpy(&transform.rotation, &normal, sizeof(glm::vec3));
		transform.scale = glm::vec3(radius);
		type = Entity::Type::Disk;
	}

	bool Intersect(const glm::vec3& ro, const glm::vec3& rd, float& depth, uint32_t& data) const { return Intersect(ro, rd, depth); }
	bool Intersect(const glm::vec3& ro, const glm::vec3& rd, float& depth) const {
		glm::vec3 o = ro - transform.position;
		depth = -glm::dot(Normal(), o) / glm::dot(rd, Normal());
		if (depth < 0.0f) return false;
		glm::vec3  q = o + rd * depth;
		if (dot(q, q) < transform.scale.x * transform.scale.x) return true;
		return false;
	}

	float EstimatedDistanceTo(const glm::vec3& pos) const {
		auto os = pos - this->transform.position;
		os = Utils::Project(os, Normal());
		return glm::length(os);
	}

	Color GetColor(const glm::vec3& pos, const uint32_t& extraData) const {

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

	Box(const glm::vec3& pos, const float& radius) {
		transform.position = pos;
		transform.scale = glm::vec3(radius);
		aabb = AABB(pos - radius, pos + radius);
		type = Entity::Type::Box;
	}

	bool Intersect(const glm::vec3& ro, const glm::vec3& rd, float& depth, uint32_t& data) const { return Intersect(ro, rd, depth); }
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

	Color GetColor(const glm::vec3& pos, const uint32_t& extraData) const {
	
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
