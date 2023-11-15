#include "Light.h"

#include "Engine/Utils.h"

float Light::CalcNL(const glm::vec3& toLight, const glm::vec3& nrm) const {
	//return glm::dot(toLight, nrm) * 0.5f + 0.5f;
	return glm::max(glm::dot(toLight, nrm), 0.0f);
}

float Light::CalcAttenuation(const float& dist) const {
	return 1.0f - Utils::InvLerpClamp(dist, 0.0f, range); // Linear falloff
}

void Light::CalcGenericLighting(const glm::vec3& pos, const glm::vec3& nrm, float& attenuation, float& nl) const {
	using namespace glm;
	vec3 os = position - pos;
	float distSqr = dot(os, os);
	float dist = sqrt(distSqr);
	os /= dist + 0.000001f; // Normalize
	nl = CalcNL(os, nrm);
	attenuation = CalcAttenuation(dist);
}
