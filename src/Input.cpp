#include "Input.h"

#include "Time.h"

std::unordered_map<SDL_Keycode, Input::KeyState> Input::keyStates;
std::unordered_map<Uint8, Input::KeyState> Input::mouseStates;
glm::vec2 Input::mouseWheel;
glm::vec2 Input::prevMouse;
int Input::mouseDelta[2] = { 0, 0 };
int Input::mousePos[2] = { 0, 0 };

void Input::UpdateKeys() {

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

bool Input::OnKeyDown(SDL_Keycode key, bool raw) {
	auto iter = keyStates.find(key);
	if (iter == keyStates.end()) return false;
	return raw ? iter->second.downRaw == Time::time : iter->second.down == Time::time;
}

bool Input::OnKeyUp(SDL_Keycode key, bool raw) {
	auto iter = keyStates.find(key);
	if (iter == keyStates.end()) return false;
	return raw ? iter->second.upRaw == Time::time : iter->second.up == Time::time;
}

bool Input::KeyHeld(SDL_Keycode key, bool raw) {
	auto iter = keyStates.find(key);
	if (iter == keyStates.end()) return false;
	auto up = raw ? iter->second.upRaw : iter->second.up;
	auto down = raw ? iter->second.downRaw : iter->second.down;
	if (up == down && up < Time::time) return false; // Key was pressed and released on the same frame and that frame has gone
	return down >= up;
}

bool Input::OnMouseDown(Uint8 SDL_Button, bool raw) {
	auto iter = mouseStates.find(SDL_Button);
	if (iter == mouseStates.end()) return false;
	return raw ? iter->second.downRaw == Time::time : iter->second.down == Time::time;
}

bool Input::OnMouseUp(Uint8 SDL_Button, bool raw) {
	auto iter = mouseStates.find(SDL_Button);
	if (iter == mouseStates.end()) return false;
	return raw ? iter->second.upRaw == Time::time : iter->second.up == Time::time;
}

bool Input::MouseHeld(Uint8 SDL_Button, bool raw) {
	auto iter = mouseStates.find(SDL_Button);
	if (iter == mouseStates.end()) return false;
	auto up = raw ? iter->second.upRaw : iter->second.up;
	auto down = raw ? iter->second.downRaw : iter->second.down;
	if (up == down && up < Time::time) return false;
	return down >= up;
}

void Input::DevalidateMouse(Uint8 SDL_Button) {
	auto iter = mouseStates.find(SDL_Button);
	if (iter == mouseStates.end()) return;
	iter->second.down = -999999.9;
	iter->second.up = -888888.8;
}

glm::vec2 Input::GetMouseWheel() {
	// @TODO: Devalidation for wheel?
	return mouseWheel;
}
