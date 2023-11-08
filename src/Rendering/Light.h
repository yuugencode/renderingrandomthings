#pragma once

#include <glm/glm.hpp>

#include "Engine/BvhPoint.h"

struct Light {
	glm::vec3 position;
	glm::vec3 color;
	float range, intensity;

	// Struct for querying positions this light hits, useful for soft shadows, gameplay..
	BvhPoint shadowBvh;

	std::vector<glm::vec4> _shadowBuffer; // Temp points to be added to shadowbvh every frame

	// Calculates boring lighting for given pos/nrm
	// @TODO: Exciting lighting instead
	void CalcGenericLighting(const glm::vec3& pos, const glm::vec3& nrm, float& attenuation, float& nl) const;
};
