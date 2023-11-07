#pragma once

#include <sdl/SDL.h>
#include <glm/vec2.hpp>

#include <unordered_map>

// Handles generic input from keyboard/mouse 
class Input {
public:
	struct KeyState {
		double down, up;
		double downRaw, upRaw; // Raw values, can't be invalidated
		auto operator<=>(const KeyState&) const = default; // Not used atm but gives easy == != impl
	};

	static int mouseDelta[2];
	static int mousePos[2];

	// Updates the keys/mouse down information for this frame 
	static void UpdateKeys();

	// Returns true for the frame a key was pressed down 
	static bool OnKeyDown(SDL_Keycode key, bool raw = false);
	
	// Returns true for the frame a key was released 
	static bool OnKeyUp(SDL_Keycode key, bool raw = false);
	
	// Returns true while a key is pressed down 
	static bool KeyHeld(SDL_Keycode key, bool raw = false);
	
	// Returns true for the frame a mouse button was pressed down 
	static bool OnMouseDown(Uint8 SDL_Button, bool raw = false);
	
	// Returns true for the frame a mouse button was released 
	static bool OnMouseUp(Uint8 SDL_Button, bool raw = false);
	
	// Returns true while a mouse button is pressed down 
	static bool MouseHeld(Uint8 SDL_Button, bool raw = false);
	
	// Devalidates mouse input, making it return false. Raw input always returns "ground-truth" 
	static void DevalidateMouse(Uint8 SDL_Button);

	// Returns mouse wheel for the current frame 
	static glm::vec2 GetMouseWheel();

private:
	Input() {}

	static std::unordered_map<SDL_Keycode, KeyState> keyStates;
	static std::unordered_map<Uint8, KeyState> mouseStates;
	static glm::vec2 mouseWheel;
	static glm::vec2 prevMouse;
};
