#pragma once

#include <glm/glm.hpp>

struct Light {
	glm::vec3 position;
	glm::vec3 color;
	float range, intensity;

	// Calculates boring lighting for given pos/nrm
	// @TODO: Exciting lighting instead
	void CalcGenericLighting(const glm::vec3& pos, const glm::vec3& nrm, float& attenuation, float& nl) const;
};
