export module Assets;

import <memory>;
import <vector>;
import <filesystem>;
import Texture;
import Mesh;

export class Assets {
public:
	inline static std::vector<std::unique_ptr<Texture>> Textures;
	inline static std::vector<std::unique_ptr<Mesh>> Meshes;

	// Shorthands for registering assets that return the handle id
	static int NewTexture(const std::filesystem::path& path) {
		auto ptr = std::make_unique<Texture>(path);
		Textures.push_back(std::move(ptr));
		return (int)Textures.size() - 1;
	}
	static int NewMesh(const std::filesystem::path& path) {
		auto ptr = std::make_unique<Mesh>(path);
		Meshes.push_back(std::move(ptr));
		return (int)Meshes.size() - 1;
	}
};

