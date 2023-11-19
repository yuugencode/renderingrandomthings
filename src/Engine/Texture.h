#pragma once

#include <glm/glm.hpp>
#include <filesystem>
#include <vector>

#include "Engine/Common.h"

// Texture loaded as RGBA8
class Texture {
public:

	// Data gets converted to standard Color vector on load
	std::vector<Color> data;
	glm::ivec2 size;
	glm::vec2 sizeF;

	// Has anything been loaded?
	bool Exists() const;

	// Samples this texture with the given UV coordinates
	Color SampleUVClamp(const glm::vec2& uv) const;

	// Loads a new texture from disk, optionally flips the Y
	Texture(const std::filesystem::path& path, bool flip = false);
};
