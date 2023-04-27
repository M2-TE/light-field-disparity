#pragma once

#include "window/window.hpp"
#include "window/input.hpp"
#include "device/device_manager.hpp"
#include "renderer/renderer.hpp"

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
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
		
		ImGui::Begin("Render Info");
		ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();

		if (!poll_inputs()) return false;
		renderer.render(deviceManager.get_device_wrapper());
		
		// ImGui end
		ImGui::EndFrame();

		return true;
	}

	bool poll_inputs() {
		input.flush();
		SDL_Event sdlEvent;
		while (SDL_PollEvent(&sdlEvent)) {

			ImGui_ImplSDL2_ProcessEvent(&sdlEvent);

			switch (sdlEvent.type) {
				case SDL_QUIT: return false;

				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP: input.register_mouse_button_event(sdlEvent.button); break;
				case SDL_MOUSEMOTION: input.register_mouse_motion_event(sdlEvent.motion); break;

				case SDL_KEYUP:
				case SDL_KEYDOWN: input.register_keyboard_event(sdlEvent.key); break;

				case SDL_WINDOWEVENT: {
					//print_event(&event);
					switch (sdlEvent.window.event) {
						// case SDL_WINDOWEVENT_MINIMIZED:
						// case SDL_WINDOWEVENT_FOCUS_LOST: bPaused = true; break;
						// case SDL_WINDOWEVENT_RESTORED: bPaused = false; break;
					}
				}
			}
		}
		return true;
	}

private:
	Window window;
	Input input;
	DeviceManager deviceManager;
	Renderer renderer;
};