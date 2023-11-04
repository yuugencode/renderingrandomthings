#pragma once

#include <glm/glm.hpp>

#include "Utils.h"
#include "Entity.h"
#include "Common.h"

// Parametrized sphere
struct Sphere : Entity {
public:
	Sphere(const glm::vec3& pos, const float& radius);
	bool IntersectLocal(const Ray& ray, glm::vec3& normal, uint32_t& data, float& depth) const;
	glm::vec3 LocalNormal(const glm::vec3& pos) const;
	v2f VertexShader(const glm::vec3& worldPos, const glm::vec3& localPos, const glm::vec3& localFaceNormal, const uint32_t& data) const;
};

// Parametrized infinite plane
struct Disk : Entity {
public:
	glm::vec3 LocalNormal() const;
	Disk(const glm::vec3& pos, const glm::vec3& normal, const float& radius);
	bool IntersectLocal(const Ray& ray, glm::vec3& normal, uint32_t& data, float& depth) const;
	v2f VertexShader(const glm::vec3& worldPos, const glm::vec3& localPos, const glm::vec3& localFaceNormal, const uint32_t& data) const;
};

// An important box
struct Box : Entity {
public:
	Box(const glm::vec3& pos, const glm::vec3& size);
	bool IntersectLocal(const Ray& ray, glm::vec3& normal, uint32_t& data, float& depth) const;
	glm::vec3 LocalNormal(const glm::vec3& pos) const;
	v2f VertexShader(const glm::vec3& worldPos, const glm::vec3& localPos, const glm::vec3& localFaceNormal, const uint32_t& data) const;
};

