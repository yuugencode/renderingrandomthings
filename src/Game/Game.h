#pragma once

#include "Boilerplate/Window.h"
#include "Rendering/Raytracer.h"
#include "Game/Scene.h"

// Game related globals
namespace Game {

	// SDL Window object
	inline Window window;

	// Currently active scene
	inline Scene scene;

	// Raytracer used to render the scene
	inline Raytracer raytracer;
};
