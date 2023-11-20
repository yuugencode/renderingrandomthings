#include "Game/Scene.h"

#include <ppl.h> // Parallel for

#include "Game/Game.h"
#include "Game/Shapes.h"
#include "Game/RenderedMesh.h"
#include "Engine/Log.h"

// Temp function for adding test stuff
void Scene::ReadAndAddTestObjects() {

	// Lights
	Game::scene.lights.push_back(Light{ .position = glm::vec3(5.0f, 5.5f,  4.3f), .color = glm::vec3(1.0f, 0.9f, 0.9f), .range = 15.0f, .intensity = 1.0f });
	Game::scene.lights.push_back(Light{ .position = glm::vec3(4.0f, 5.5f, -7.0f), .color = glm::vec3(0.8f, 1.0f, 1.0f), .range = 15.0f, .intensity = 1.0f });
	//Game::scene.lights.push_back(Light{ .position = glm::vec3(-4.0f, 5.5f, -7.0f), .color = glm::vec3(0.8f, 1.0f, 0.7f), .range = 15.0f, .intensity = 1.0f });
	//Game::scene.lights.push_back(Light{ .position = glm::vec3(0.0f, 7.0f, 0.0f), .color = glm::vec3(1.0f, 1.0f, 1.0f), .range = 15.0f, .intensity = 1.0f });

#if true // Parametric shapes
	Game::scene.entities.push_back(std::make_unique<Disk>(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 20.0f));
	Game::scene.entities.back()->shaderType = Shader::Grid;

	Game::scene.entities.push_back(std::make_unique<Sphere>(glm::vec3(10.0f, 0.5f, 0.0f), 0.1f));
	Game::scene.entities.back()->materials[0].reflectivity = 0.3f;
	
	Game::scene.entities.push_back(std::make_unique<Sphere>(glm::vec3(0.0f, 1.5f, -5.0f), 2.0f));
	Game::scene.entities.back()->materials[0].reflectivity = 0.3f;
	
	Game::scene.entities.push_back(std::make_unique<Sphere>(glm::vec3(1.0f, 0.5f, 7.0f), 1.0f));
	Game::scene.entities.back()->materials[0].reflectivity = 1.0f;
	
	Game::scene.entities.push_back(std::make_unique<Box>(glm::vec3(-2.0f, 0.5f, 0.0f), glm::vec3(1.0f, 4.0f, 0.05f)));
	Game::scene.entities.back()->materials[0].reflectivity = 0.5f;
	
	Game::scene.entities.push_back(std::make_unique<Box>(glm::vec3(6.0f, 0.5f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f)));
	Game::scene.entities.back()->materials[0].color = glm::vec4(1.0f, 0.2f, 0.2f, 1.0f);
#endif

#if true // Mesh
	{
		auto meshHandle = Assets::NewMesh(std::filesystem::path("models/char.fbx"));
		auto rendMesh = std::make_unique<RenderedMesh>("chara", meshHandle);

		// Mesh 1 textures
		for (const auto& metadata : rendMesh->GetMesh()->materialMetadata) {
			auto file = metadata.textureFilename;
			Log::Line(file);
			auto path = std::filesystem::path("models") / file;
			if (file != "" && std::filesystem::exists(path))
				rendMesh->materials.push_back(Material{
					.textureHandle = Assets::NewTexture(path, Assets::ImportOpts{.flipY = true })
				});
			else
				rendMesh->materials.push_back(Material());
		}

		rendMesh->shaderType = Shader::Textured;
		rendMesh->transform.rotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1, 0, 0));
		
		Game::scene.entities.push_back(std::move(rendMesh));
	}
#endif

#if false // Mesh
	{
		auto meshHandle = Assets::NewMesh(std::filesystem::path("models/bunny.obj"));
		auto rendMesh = std::make_unique<RenderedMesh>("bunny", meshHandle);

		rendMesh->transform.scale = glm::vec3(20.0f);
		rendMesh->transform.position += glm::vec3(3.0f, -0.6f, 0.0f);
		rendMesh->transform.LookAtDir(glm::vec3(-1, 0, 0), glm::vec3(0, 1, 0));
		rendMesh->shaderType = Shader::PlainWhite;

		Game::scene.entities.push_back(std::move(rendMesh));
	}
#endif

#if false // Mesh
	{
		auto meshHandle = Assets::NewMesh(std::filesystem::path("models/dragon_vrip.obj"));
		auto rendMesh = std::make_unique<RenderedMesh>("dragon", meshHandle);

		rendMesh->transform.scale = glm::vec3(10.0f);
		rendMesh->transform.position += glm::vec3(4.0f, -0.5f, 0.0f);
		rendMesh->shaderType = Shader::PlainWhite;
		
		Game::scene.entities.push_back(std::move(rendMesh));
	}
#endif

#if false // Mesh
	{
		auto meshHandle = Assets::NewMesh(std::filesystem::path("models/armadillo.obj"));
		auto rendMesh = std::make_unique<RenderedMesh>("arma", meshHandle);

		rendMesh->transform.scale = glm::vec3(0.02f);
		rendMesh->transform.position += glm::vec3(0.0f, 1.1f, 3.0f);
		rendMesh->transform.LookAtDir(glm::vec3(1, 0, 0), glm::vec3(0, 1, 0));
		rendMesh->shaderType = Shader::PlainWhite;
	
		Game::scene.entities.push_back(std::move(rendMesh));
	}
#endif

#if false // Mesh
	{
		auto meshHandle = Assets::NewMesh(std::filesystem::path("models/sponza/sponza.fbx"), Assets::ImportOpts{ .ignoreMaterials = { 14, 28, 32, 39 } });
		auto rendMesh = std::make_unique<RenderedMesh>("sponza", meshHandle);

		// Sometimes normals in slot0, just hardcode basecolor instead
		for (auto& a : Assets::Meshes[meshHandle]->materialMetadata) {
			std::string& file = a.textureFilename;
			const std::string str = "Normal";
			auto pos = file.find(str);
			if (pos != std::string::npos) file = file.replace(pos, str.length(), "BaseColor");
			//Log::Line(a.materialName, a.textureFilename);
		}

		// Read textures multithreaded
		rendMesh->materials.resize(Assets::Meshes[meshHandle]->materialMetadata.size());
		concurrency::parallel_for(size_t(0), Assets::Meshes[meshHandle]->materialMetadata.size(), [&](size_t i) {
			const auto& file = Assets::Meshes[meshHandle]->materialMetadata[i].textureFilename;
			auto path = std::filesystem::path("models/sponza/textures") / file;
			if (file != "" && std::filesystem::exists(path))
				rendMesh->materials[i] = Material{ .textureHandle = Assets::NewTexture(path, Assets::ImportOpts{.flipY = true }) };
			else
				rendMesh->materials[i] = Material();
		});

		rendMesh->shaderType = Shader::Textured;
		Game::scene.entities.push_back(std::move(rendMesh));
	}
#endif

#if false // Mesh
	{
		auto meshHandle = Assets::NewMesh(std::filesystem::path("models/triangleBall2.fbx"));
		auto rendMesh = std::make_unique<RenderedMesh>("ball", meshHandle);
		
		rendMesh->materials[0].reflectivity = 0.5f;
		rendMesh->materials[0].color = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
		rendMesh->transform.scale = glm::vec3(2.0f);
		rendMesh->transform.position += glm::vec3(-4.0f, 2.0f, 3.0f);
		rendMesh->transform.LookAtDir(glm::vec3(-1, 0, 0), glm::vec3(0, 1, 0));
		rendMesh->shaderType = Shader::PlainWhite;

		Game::scene.entities.push_back(std::move(rendMesh));
	}
#endif
}

Entity* Scene::GetObjectByName(const std::string& name) const {
	for (const auto& obj : entities)
		if (obj->name == name) return obj.get();
	return nullptr;
}
