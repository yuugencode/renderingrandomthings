module;

export module Game;

import <memory>;
export import Window;
export import Log;
export import Utils;
export import Scene;
export import Time;
export import Entity;
export import Input;
export import Shapes;
export import RenderedMesh;
export import Transform;
export import Camera;

/// <summary> Top level class containing references to many common game functions </summary>
export class Game {
	Game() {}; ~Game() {};
public:
	inline static Window window;
	inline static Scene rootScene;
	inline static Camera camera;
};
