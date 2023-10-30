module;

export module Scene;

import <vector>;
import <memory>;
import Entity;
import Camera;

export class Scene {

public:
	
	// The scene is a flat structure rather than a tree for now
	std::vector<std::unique_ptr<Entity>> entities;
	
	Camera camera;
	
	Scene() {
		entities.reserve(32);
	}
};