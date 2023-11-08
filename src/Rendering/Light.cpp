#include "Light.h"

#include "Engine/Utils.h"

void Light::CalcGenericLighting(const glm::vec3& pos, const glm::vec3& nrm, float& attenuation, float& nl) const {
	using namespace glm;
	vec3 os = position - pos;
	float distSqr = dot(os, os);
	float dist = sqrt(distSqr);
	os /= dist + 0.000001f; // Normalize
	nl = dot(os, nrm) * 0.5f + 0.5f;
	//attenuation = clamp((range / distSqr) * intensity, 0.0f, 1.0f);
	attenuation = 1.0f - Utils::InvLerpClamp(dist, 0.0f, range); // Linear falloff
}
