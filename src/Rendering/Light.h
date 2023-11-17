#pragma once

#include <glm/glm.hpp>

#include "Engine/Common.h"
#include "Engine/BvhPoint.h"

struct Light {
	glm::vec3 position;
	glm::vec3 color;
	float range, intensity;

	// Struct for querying positions this light hits, useful for soft shadows, gameplay etc..
	BvhPoint<Empty> lightBvh;
	std::vector<glm::vec4> _lightBvhTempBuffer; // Temp points to be added to shadowbvh every frame

	// Indirect buffer contents
	struct BufferPt {
		glm::vec3 pt;
		Color clr; // Indirect color for pos
		glm::ivec2 ids; // Object IDs that cast this indirect, used for filtering self-lighting
	};

	std::vector<BufferPt> _indirectTempBuffer; // Temp points to be added to indirect buffer every frame
	std::vector<BufferPt> _toAddBuffer; // Reduced size buffer without invalid values to be passed to bvh constructor

	BvhPoint<glm::vec3> indirectBvh; // Contains points representing indirect light this light emits

	// Calculates boring lighting for given pos/nrm
	// @TODO: Exciting lighting instead
	void CalcGenericLighting(const glm::vec3& pos, const glm::vec3& nrm, float& attenuation, float& nl) const;

	float CalcNL(const glm::vec3& toLight, const glm::vec3& nrm) const;

	float CalcAttenuation(const float& dist) const;
};
