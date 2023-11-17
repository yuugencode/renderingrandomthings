#pragma once

#include <glm/glm.hpp>

#include "Engine/Common.h"

struct Material {
	int textureHandle = -1;
	float reflectivity = 0.0f;
	glm::vec4 color = glm::vec4(1.0f);
	bool HasTexture() { return textureHandle != -1; }
};