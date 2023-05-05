#pragma once

class Input
{
public:
	Input() = default;
	~Input() = default;

public:
	// per-frame
	void flush() {
		xMouseRel = yMouseRel = 0;
		keysPressed.clear();
		keysReleased.clear();
		mouseButtonsPressed.clear();
		mouseButtonsReleased.clear();
	}
	// e.g. on window exit
	void flush_all() {
		flush();
		keysDown.clear();
		mouseButtonsDown.clear();
	}

	// keyboard
	void register_keyboard_event(SDL_KeyboardEvent keyEvent) {
		// do not register repeat inputs and ignore when imgui already consumes them
		if (keyEvent.repeat || ImGui::GetIO().WantCaptureKeyboard) return;

		switch (keyEvent.type) {
			case SDL_EVENT_KEY_DOWN: {
				keysPressed.insert(keyEvent.keysym.sym);
				keysDown.insert(keyEvent.keysym.sym);
				break;
			}
			case SDL_EVENT_KEY_UP: {
				keysReleased.insert(keyEvent.keysym.sym);
				keysDown.erase(keyEvent.keysym.sym);
				break;
			}
		}
	}
	
	// mouse
	void register_mouse_button_event(SDL_MouseButtonEvent mbEvent) {
		// ignore mouse inputs when imgui wants to capture
		if (ImGui::GetIO().WantCaptureMouse) return;

		switch (mbEvent.type) {
			case SDL_EVENT_MOUSE_BUTTON_DOWN: {
				mouseButtonsPressed.insert(mbEvent.button);
				mouseButtonsDown.insert(mbEvent.button);
				break;
			}
			case SDL_EVENT_MOUSE_BUTTON_UP: {
				mouseButtonsReleased.insert(mbEvent.button);
				mouseButtonsDown.erase(mbEvent.button);
				break;
			}
		}
	}
	void register_mouse_motion_event(SDL_MouseMotionEvent mmEvent) {
		xMouseRel += mmEvent.xrel;
		yMouseRel += mmEvent.yrel;
		xMouse += mmEvent.xrel;
		yMouse += mmEvent.yrel;
	}

public:
	Sint32 xMouseRel, yMouseRel;
	Sint32 xMouse, yMouse;

	std::set<uint8_t> mouseButtonsPressed;
	std::set<uint8_t> mouseButtonsDown;
	std::set<uint8_t> mouseButtonsReleased;

	std::set<SDL_Keycode> keysPressed;
	std::set<SDL_Keycode> keysDown;
	std::set<SDL_Keycode> keysReleased;
};
