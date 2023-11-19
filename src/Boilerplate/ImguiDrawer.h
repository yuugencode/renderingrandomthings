#pragma once

#include <bgfx/bgfx.h>
#include <imgui/imgui.h>

#include <filesystem>

// Contains helper functions to work with imgui functions 
class ImguiDrawer {

    static bool initialized;
	static bgfx::VertexLayout  vertexLayout;
	static bgfx::TextureHandle fontTexture;
	static bgfx::UniformHandle fontUniform;
	static bgfx::ProgramHandle program;

    ImguiDrawer() {}
    ~ImguiDrawer() {}

public:

	static inline const bgfx::ViewId VIEW_LAYER = 1;
	
	// Initializes Imgui stuff, loads font etc
	static void Init();

	// Starts a new Imgui frame
	static void NewFrame();

	// Draws the default debug UI
	static void DrawUI();
	
	// Renders the UI in backbuffer
	// Loosely based on now seemingly unmaintained bigg-library's public domain implementation
	static void Render();

	// Unloads Imgui
	static void Quit();
};

