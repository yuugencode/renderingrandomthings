#pragma once

#include <glm/glm.hpp>
#include <filesystem>
#include <vector>

#include "Common.h"

// Texture loaded as RGBA8
class Texture {
public:

	// Data gets converted to standard Color vector on load
	std::vector<Color> data;
	int width = 0, height = 0;
	
	// Has anything been loaded?
	bool Exists() const;

	// Samples this texture with the given UV coordinates
	Color SampleUVClamp(const glm::vec2& uv) const;

	// Loads a new texture from disk, optionally flips the Y
	Texture(const std::filesystem::path& path, bool flip = false);
};
