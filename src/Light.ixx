module;

#include <glm/glm.hpp>

export module Light;

export class Light {
public:

	glm::vec3 position;
	float range, invRange, intensity;

	Light(const glm::vec3& position, const float& range, const float& intensity) {
		this->range = range;
		this->intensity = intensity;
		this->position = position;
		invRange = 1.0f / range;
	}

	// Calculates boring lighting for given pos/nrm
	void CalcGenericLighting(const glm::vec3& pos, const glm::vec3& nrm, float& attenuation, float& nl) const {
		using namespace glm;
		vec3 os = position - pos;
		float distSqr = dot(os, os);
		os /= sqrt(distSqr) + 0.000001f;
		nl = dot(os, nrm) * 0.5f + 0.5f;
		attenuation = clamp((range / distSqr) * intensity, 0.0f, 1.0f);
	}
};