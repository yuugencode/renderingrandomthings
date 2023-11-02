module;

export module Scene;

import <vector>;
import <memory>;
import Entity;
import Shapes;
import RenderedMesh;
import Camera;
import Light;

export class Scene {

public:
	
	// The scene is a flat structure rather than a tree for now
	std::vector<std::unique_ptr<Entity>> entities;
	std::vector<Light> lights;
	Camera camera;
};