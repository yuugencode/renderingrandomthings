#pragma once

#include <glm/glm.hpp>

class Light {
public:
	glm::vec3 position;
	float range, invRange, intensity;
	Light(const glm::vec3& position, const float& range, const float& intensity);

	// Calculates boring lighting for given pos/nrm
	// @TODO: Exciting lighting instead
	void CalcGenericLighting(const glm::vec3& pos, const glm::vec3& nrm, float& attenuation, float& nl) const;
};
