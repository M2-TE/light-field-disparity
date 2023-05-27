#pragma once

#include "window/window.hpp"
#include "window/input.hpp"
#include "device/device_manager.hpp"
#include "renderer/renderer.hpp"
#include "renderer/push_constants.hpp"

class Application
{
public:
	Application() {
		VMI_LOG("[Initializing] Independent vulkan functions...");
		vk::DynamicLoader dl;
		PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
		VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

		window.init(512, 512);
		deviceManager.init(window.get_vulkan_instance(), window.get_vulkan_surface());
		renderer.init(deviceManager.get_device_wrapper(), window);
		VMI_LOG("[Initialization Complete]" << std::endl);
	}
	~Application() {
		deviceManager.get_logical_device().waitIdle();
		renderer.destroy(deviceManager.get_device_wrapper());

		deviceManager.destroy();
		window.destroy();
	}

public:
	void run() {
		while (update()) {}
	}

private:
	bool update() {
		// ImGui begin
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();
		
		if (!poll_inputs()) return false;
		handle_inputs();
		draw_ui();

		renderer.render(deviceManager.get_device_wrapper(), pcs);
		
		// ImGui end
		ImGui::EndFrame();

		return true;
	}
	bool poll_inputs() {
		input.flush();
		SDL_Event sdlEvent;
		while (SDL_PollEvent(&sdlEvent)) {

			ImGui_ImplSDL3_ProcessEvent(&sdlEvent);

			switch (sdlEvent.type) {
				case SDL_EVENT_QUIT: return false;

				case SDL_EVENT_MOUSE_BUTTON_DOWN:
				case SDL_EVENT_MOUSE_BUTTON_UP: input.register_mouse_button_event(sdlEvent.button); break;
				case SDL_EVENT_MOUSE_MOTION: input.register_mouse_motion_event(sdlEvent.motion); break;

				case SDL_EVENT_KEY_UP:
				case SDL_EVENT_KEY_DOWN: input.register_keyboard_event(sdlEvent.key); break;
			}
		}
		return true;
	}
	void handle_inputs() {
		if (input.keysPressed.count(SDLK_1)) pcs.nSteps = 1;
		if (input.keysPressed.count(SDLK_2)) pcs.nSteps = 2;
		if (input.keysPressed.count(SDLK_3)) pcs.nSteps = 3;
		if (input.keysPressed.count(SDLK_4)) pcs.nSteps = 4;
		if (input.keysPressed.count(SDLK_5)) pcs.nSteps = 5;
		if (input.keysPressed.count(SDLK_6)) pcs.nSteps = 6;
		if (input.keysPressed.count(SDLK_7)) pcs.nSteps = 7;
		if (input.keysPressed.count(SDLK_8)) pcs.nSteps = 8;
		if (input.keysPressed.count(SDLK_9)) pcs.nSteps = 9;
	}
	void draw_ui() {
		ImGui::Begin("Render Info");
		ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}

private:
	Window window;
	Input input;
	DeviceManager deviceManager;
	Renderer renderer;
	PushConstants pcs;
};