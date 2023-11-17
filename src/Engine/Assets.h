#pragma once

#include <memory>
#include <vector>
#include <filesystem>

#include "Engine/Texture.h"
#include "Engine/Mesh.h"

// Static class providing indexed access to loaded meshes and textures
class Assets {
	Assets(){}
public:
	static std::vector<std::unique_ptr<Texture>> Textures;
	static std::vector<std::unique_ptr<Mesh>> Meshes;

	struct ImportOpts {
		bool flipY = false;
		bool loadMtl = false;
		std::vector<int> ignoreMaterials;
	};

	// Reads a texture and returns its handle
	static int NewTexture(const std::filesystem::path& path, const ImportOpts& opts = ImportOpts());
	
	// Reads a mesh file into a mesh and returns its handle
	static int NewMesh(const std::filesystem::path& path, const ImportOpts& opts = ImportOpts());
	
	// Reads a mesh file and splits every submesh it has into its own mesh object
	static void NewMeshes(const std::filesystem::path& path, std::vector<int>& meshHandles, const ImportOpts& opts = ImportOpts());
};
