#pragma once

#include <memory>
#include <vector>
#include <filesystem>

#include "Texture.h"
#include "Mesh.h"

class Assets {
public:
	static std::vector<std::unique_ptr<Texture>> Textures;
	static std::vector<std::unique_ptr<Mesh>> Meshes;

	static int NewTexture(const std::filesystem::path& path, bool flipY = false);
	static int NewMesh(const std::filesystem::path& path);
};