#pragma once

#include <glm/glm.hpp>

#include "Engine/Common.h"
#include "Game/Entity.h"

// Parametrized sphere
struct Sphere : Entity {
public:
	Sphere(const glm::vec3& pos, const float& radius);
	bool IntersectLocal(const Ray& ray, glm::vec3& normal, int& data, float& depth) const;
	glm::vec3 LocalNormal(const glm::vec3& pos) const;
	v2f VertexShader(const Ray& ray, const RayResult& rayResult) const;
};

// Parametrized infinite plane
struct Disk : Entity {
public:
	Disk(const glm::vec3& pos, const glm::vec3& normal, const float& radius);
	bool IntersectLocal(const Ray& ray, glm::vec3& normal, int& data, float& depth) const;
	v2f VertexShader(const Ray& ray, const RayResult& rayResult) const;
};

// An important box
struct Box : Entity {
public:
	Box(const glm::vec3& pos, const glm::vec3& size);
	bool IntersectLocal(const Ray& ray, glm::vec3& normal, int& data, float& depth) const;
	glm::vec3 LocalNormal(const glm::vec3& pos) const;
	v2f VertexShader(const Ray& ray, const RayResult& rayResult) const;
};

