#pragma once

#include <glm/glm.hpp>

#include "Engine/Common.h"
#include "Engine/BvhPoint.h"

struct Light {
	glm::vec3 position;
	glm::vec3 color;
	float range, intensity;

	// Struct for querying positions this light hits, useful for soft shadows, gameplay..
	BvhPoint<Empty> shadowBvh;
	std::vector<glm::vec4> _shadowTempBuffer; // Temp points to be added to shadowbvh every frame

	struct BufferPt {
		glm::vec3 pt;
		Color clr;
		glm::ivec2 ids;
	};

	std::vector<BufferPt> _toAddBuffer; // Reduced size buffer without invalid values to be passed to bvh
	std::vector<BufferPt> _indirectTempBuffer; // Temp points to be added to indirect buffer every frame
	BvhPoint<glm::vec3> lightBvh; // Contains points representing indirect light this light emits

	// Calculates boring lighting for given pos/nrm
	// @TODO: Exciting lighting instead
	void CalcGenericLighting(const glm::vec3& pos, const glm::vec3& nrm, float& attenuation, float& nl) const;

	float CalcNL(const glm::vec3& toLight, const glm::vec3& nrm) const;

	float CalcAttenuation(const float& dist) const;
};
