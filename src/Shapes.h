#pragma once

#include <glm/glm.hpp>

#include "Utils.h"
#include "Entity.h"

// Parametrized sphere
struct Sphere : Entity {
public:
	Sphere(const glm::vec3& pos, const float& radius);
	bool Intersect(const Ray& ray, glm::vec3& normal, uint32_t& data, float& depth) const;
	float EstimatedDistanceTo(const glm::vec3& pos) const;
	glm::vec3 Normal(const glm::vec3& pos) const;
	v2f VertexShader(const glm::vec3& pos, const glm::vec3& faceNormal, const uint32_t& data) const;
};

// Parametrized infinite plane
struct Disk : Entity {
public:
	glm::vec3& Normal() const;
	Disk(const glm::vec3& pos, const glm::vec3& normal, const float& radius);
	bool Intersect(const Ray& ray, glm::vec3& normal, uint32_t& data, float& depth) const;
	float EstimatedDistanceTo(const glm::vec3& pos) const;
	v2f VertexShader(const glm::vec3& pos, const glm::vec3& faceNormal, const uint32_t& data) const;
};

// An important box
struct Box : Entity {
public:
	Box(const glm::vec3& pos, const glm::vec3& size);
	bool Intersect(const Ray& ray, glm::vec3& normal, uint32_t& data, float& depth) const;
	float EstimatedDistanceTo(const glm::vec3& pos) const;
	glm::vec3 Normal(const glm::vec3& pos) const;
	v2f VertexShader(const glm::vec3& pos, const glm::vec3& faceNormal, const uint32_t& data) const;
};

