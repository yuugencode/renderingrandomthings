module;

#include <sdl/SDL.h>
#include <glm/vec2.hpp>

export module Input;

import <unordered_map>;
import Time;

/// <summary> Handles generic input from keyboard/mouse </summary>
export class Input {

private:
	Input() {}

	struct KeyState {
		double down, up;
		double downRaw, upRaw; // Raw values, can't be invalidated
		auto operator<=>(const KeyState&) const = default; // Not used atm but gives easy == != impl
	};

	inline static std::unordered_map<SDL_Keycode, KeyState> keyStates;
	inline static std::unordered_map<Uint8, KeyState> mouseStates;
	inline static glm::vec2 mouseWheel;

	inline static glm::vec2 prevMouse;

public:

	inline static int mouseDelta[2] = { 0, 0 };
	inline static int mousePos[2] = { 0, 0 };

	/// <summary> Updates the keys/mouse down information for this frame </summary>
	static void UpdateKeys() {

		// Mouse position
		SDL_GetMouseState(&mousePos[0], &mousePos[1]);
		mouseDelta[0] = mouseDelta[1] = 0;
		mouseWheel = glm::vec2(0, 0); // Wheel only has down events so has to be reset every frame

		// Keypresses
		SDL_Event e;
		while (SDL_PollEvent(&e)) {

			// Keyboard
			if (e.type == SDL_KEYDOWN) {
				auto iter = keyStates.find(e.key.keysym.sym);
				if (iter == keyStates.end())
					keyStates[e.key.keysym.sym] = KeyState{ .down = Time::time, .up = 0, .downRaw = Time::time, .upRaw = 0 };
				else
					iter->second.down = iter->second.downRaw = Time::time;
			}
			else if (e.type == SDL_KEYUP) {
				auto iter = keyStates.find(e.key.keysym.sym);
				if (iter == keyStates.end())
					keyStates[e.key.keysym.sym] = KeyState{ .down = 0, .up = Time::time, .downRaw = 0, .upRaw = Time::time };
				else
					iter->second.up = iter->second.upRaw = Time::time;
			}

			// Mouse
			else if (e.type == SDL_MOUSEBUTTONDOWN) {
				auto iter = mouseStates.find(e.button.button);
				if (iter == mouseStates.end())
					mouseStates[e.button.button] = KeyState{ .down = Time::time, .up = 0, .downRaw = Time::time, .upRaw = 0 };
				else
					iter->second.down = iter->second.downRaw = Time::time;
			}
			else if (e.type == SDL_MOUSEBUTTONUP) {
				auto iter = mouseStates.find(e.button.button);
				if (iter == mouseStates.end())
					mouseStates[e.button.button] = KeyState{ .down = 0, .up = Time::time, .downRaw = 0, .upRaw = Time::time };
				else
					iter->second.up = iter->second.upRaw = Time::time;
			}
			else if (e.type == SDL_MOUSEWHEEL) {
				mouseWheel = glm::vec2(e.wheel.preciseX, e.wheel.preciseY);
			}
			else if (e.type == SDL_MOUSEMOTION) {
				mouseDelta[0] += e.motion.xrel;
				mouseDelta[1] += e.motion.yrel;
			}
		}
	}

	/// <summary> Returns true for the frame a key was pressed down </summary>
	static bool OnKeyDown(SDL_Keycode key, bool raw = false) {
		auto iter = keyStates.find(key);
		if (iter == keyStates.end()) return false;
		return raw ? iter->second.downRaw == Time::time : iter->second.down == Time::time;
	}

	/// <summary> Returns true for the frame a key was released </summary>
	static bool OnKeyUp(SDL_Keycode key, bool raw = false) {
		auto iter = keyStates.find(key);
		if (iter == keyStates.end()) return false;
		return raw ? iter->second.upRaw == Time::time : iter->second.up == Time::time;
	}

	/// <summary> Returns true while a key is pressed down </summary>
	static bool KeyHeld(SDL_Keycode key, bool raw = false) {
		auto iter = keyStates.find(key);
		if (iter == keyStates.end()) return false;
		auto up = raw ? iter->second.upRaw : iter->second.up;
		auto down = raw ? iter->second.downRaw : iter->second.down;
		if (up == down && up < Time::time) return false; // Key was pressed and released on the same frame and that frame has gone
		return down >= up;
	}

	/// <summary> Returns true for the frame a mouse button was pressed down </summary>
	static bool OnMouseDown(Uint8 SDL_Button, bool raw = false) {
		auto iter = mouseStates.find(SDL_Button);
		if (iter == mouseStates.end()) return false;
		return raw ? iter->second.downRaw == Time::time : iter->second.down == Time::time;
	}

	/// <summary> Returns true for the frame a mouse button was released </summary>
	static bool OnMouseUp(Uint8 SDL_Button, bool raw = false) {
		auto iter = mouseStates.find(SDL_Button);
		if (iter == mouseStates.end()) return false;
		return raw ? iter->second.upRaw == Time::time : iter->second.up == Time::time;
	}

	/// <summary> Returns true while a mouse button is pressed down </summary>
	static bool MouseHeld(Uint8 SDL_Button, bool raw = false) {
		auto iter = mouseStates.find(SDL_Button);
		if (iter == mouseStates.end()) return false;
		auto up = raw ? iter->second.upRaw : iter->second.up;
		auto down = raw ? iter->second.downRaw : iter->second.down;
		if (up == down && up < Time::time) return false;
		return down >= up;
	}

	/// <summary> Devalidates mouse input, making it return false. Raw input always returns "ground-truth" </summary>
	static void DevalidateMouse(Uint8 SDL_Button) {
		auto iter = mouseStates.find(SDL_Button);
		if (iter == mouseStates.end()) return;
		iter->second.down = -999999.9;
		iter->second.up = -888888.8;
	}

	/// <summary> Returns mouse wheel for the current frame </summary>
	static glm::vec2 GetMouseWheel() {
		// @TODO: Devalidation for wheel?
		return mouseWheel;
	}
};