#pragma once

#include "Window.h"
#include "Scene.h"
#include "Raytracer.h"

// Game related globals
namespace Game {

	// SDL Window object
	inline Window window;

	// Currently active scene
	inline Scene scene;

	// Raytracer used to render the scene
	inline Raytracer raytracer;
};
