#include "Assets.h"

std::vector<std::unique_ptr<Texture>> Assets::Textures;
std::vector<std::unique_ptr<Mesh>> Assets::Meshes;

// Shorthands for registering assets that return the handle id
int Assets::NewTexture(const std::filesystem::path& path, const ImportOpts& opts) {
	auto ptr = std::make_unique<Texture>(path, opts.flipY);
	Textures.push_back(std::move(ptr));
	return (int)Textures.size() - 1;
}

int Assets::NewMesh(const std::filesystem::path& path, const ImportOpts& opts) {
	auto ptr = std::make_unique<Mesh>();
	ptr->ignoreMaterials = opts.ignoreMaterials;
	ptr->LoadMesh(path, opts.loadMtl);
	ptr->ReadAllNodes();
	ptr->UnloadMesh();
	Meshes.push_back(std::move(ptr));
	return (int)Meshes.size() - 1;
}

void Assets::NewMeshes(const std::filesystem::path& path, std::vector<int>& meshHandles, const ImportOpts& opts) {

	int numMeshes;
	Mesh temp;
	temp.ignoreMaterials = opts.ignoreMaterials;
	temp.LoadMesh(path);
	numMeshes = temp.GetMeshNodeCount();

	for (size_t i = 0; i < numMeshes; i++) {
		temp.ReadSceneMeshNode((int)i);
		auto ptr = std::make_unique<Mesh>();
		*ptr = temp; // Copy
		Meshes.push_back(std::move(ptr));
		meshHandles.push_back((int)Meshes.size() - 1);
	}

	temp.UnloadMesh();
}