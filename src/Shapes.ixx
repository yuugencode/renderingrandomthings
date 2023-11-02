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
		shaderType = Shader::Normals;
		id = idCount++;
	}

	bool Intersect(const Ray& ray, glm::vec3& normal, uint32_t& data, float& depth) const {
		if (ray.mask == id) return false;
		// Adapted from Inigo Quilez https://iquilezles.org/articles/
		glm::vec3 oc = ray.ro - transform.position;
		float b = glm::dot(oc, ray.rd);
		if (b > 0.0f) return false;
		float c = glm::dot(oc, oc) - transform.scale.x * transform.scale.x;
		float h = b * b - c;
		if (h < 0.0f) return false;
		h = sqrt(h);
		depth = -b - h;
		normal = glm::normalize((ray.ro + ray.rd * depth) - transform.position);
		data = id;
		return true;
	}

	float EstimatedDistanceTo(const glm::vec3& pos) const {
		return glm::max(glm::distance(pos, transform.position) - transform.scale.x, 0.0f);
	}

	glm::vec3 Normal(const glm::vec3& pos) const {
		return glm::normalize(pos - transform.position);
	}

	v2f VertexShader(const glm::vec3& pos, const glm::vec3& faceNormal, const uint32_t& data) const {
		return v2f{ .position = pos, .normal = faceNormal, .uv = glm::vec2(0), .data = data};
	}
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
		id = idCount++;
		shaderType = Shader::Grid;
	}

	bool Intersect(const Ray& ray, glm::vec3& normal, uint32_t& data, float& depth) const {
		if (ray.mask == id) return false;
		glm::vec3 o = ray.ro - transform.position;
		normal = Normal();
		depth = -glm::dot(Normal(), o) / glm::dot(ray.rd, Normal());
		data = id;
		if (depth < 0.0f) return false;
		glm::vec3  q = o + ray.rd * depth;
		if (dot(q, q) < transform.scale.x * transform.scale.x) return true;
		return false;
	}

	float EstimatedDistanceTo(const glm::vec3& pos) const {
		auto os = pos - this->transform.position;
		os = Utils::Project(os, Normal());
		return glm::length(os);
	}

	v2f VertexShader(const glm::vec3& pos, const glm::vec3& faceNormal, const uint32_t& data) const {
		return v2f{ .position = pos, .normal = faceNormal, .uv = glm::vec2(0), .data = data };
	}
};

/// <summary> A box </summary>
export struct Box : Entity {
public:

	Box(const glm::vec3& pos, const glm::vec3& size) {
		transform.position = pos;
		transform.scale = size;
		aabb = AABB(pos - size, pos + size);
		type = Entity::Type::Box;
		id = idCount++;
		shaderType = Shader::PlainWhite;
	}

	bool Intersect(const Ray& ray, glm::vec3& normal, uint32_t& data, float& depth) const {
		if (ray.mask == id) return false;
		auto res = aabb.Intersect(ray);
		if (res > 0.0f && id != ray.mask) {
			depth = res;
			normal = Normal(ray.ro + ray.rd * res);
			data = id;
			return true;
		}
		return false;
	}

	float EstimatedDistanceTo(const glm::vec3& pos) const {
		return glm::max(glm::distance(pos, aabb.Center()) - aabb.Size().x, 0.0f);
	}

	glm::vec3 Normal(const glm::vec3& pos) const {
		const auto nrm = (pos - aabb.Center()) / transform.scale;
		if (glm::abs(nrm.x) > glm::abs(nrm.y) && glm::abs(nrm.x) > glm::abs(nrm.z))
			return nrm.x < 0.0f ? glm::vec3(-1.0f, 0.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
		else if (glm::abs(nrm.y) > glm::abs(nrm.z))
			return nrm.y < 0.0f ? glm::vec3(0.0f, -1.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
		else
			return nrm.z < 0.0f ? glm::vec3(0.0f, 0.0f, -1.0f) : glm::vec3(0.0f, 0.0f, 1.0f);
	}

	v2f VertexShader(const glm::vec3& pos, const glm::vec3& faceNormal, const uint32_t& data) const {
		return v2f{ .position = pos, .normal = faceNormal, .uv = glm::vec2(0), .data = data };
	}
};
