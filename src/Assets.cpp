#include "Assets.h"

std::vector<std::unique_ptr<Texture>> Assets::Textures;
std::vector<std::unique_ptr<Mesh>> Assets::Meshes;

// Shorthands for registering assets that return the handle id
int Assets::NewTexture(const std::filesystem::path& path, bool flipY) {
	auto ptr = std::make_unique<Texture>(path, flipY);
	Textures.push_back(std::move(ptr));
	return (int)Textures.size() - 1;
}

int Assets::NewMesh(const std::filesystem::path& path) {
	auto ptr = std::make_unique<Mesh>();
	ptr->LoadMesh(path);
	ptr->ReadAllNodes();
	ptr->UnloadMesh();
	Meshes.push_back(std::move(ptr));
	return (int)Meshes.size() - 1;
}
