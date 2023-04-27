#pragma once

// already defined as typedef in linux apparently, so this is an easy substitute
#define Window Window_VMI

class Window
{
public:
	Window() = default;
	~Window() = default;
	// ROF_COPY_MOVE_DELETE(Window)

public:
	void init(int32_t width, int32_t height) {
		VMI_LOG("[Initializing] SDL window...");
		init_sdl_window(width, height);
		SDL_version version;
		SDL_GetVersion(&version);
		std::string spacing = "    ";
		VMI_LOG(spacing << "SDL version: " << (uint32_t)version.major << "." << (uint32_t)version.minor << "." << (uint32_t)version.patch);

		VMI_LOG("[Initializing] Vulkan instance...");
		create_vulkan_instance();
		create_vulkan_surface();

		VMI_LOG("[Initializing] Instance-specific vulkan functions...");
		VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);

		VMI_LOG("[Initializing] ImGui...");
		ImGui::CreateContext();
		ImGui_ImplSDL2_InitForVulkan(pWindow);
		ImGui::StyleColorsDark();
		std::string imguiVer = ImGui::GetVersion();
		VMI_LOG(spacing << "ImGui version: " << imguiVer);
	}
	void destroy() {
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();

		SDL_DestroyWindow(pWindow);
		SDL_Quit();

		instance.destroySurfaceKHR(surface);
		DEBUG_ONLY(instance.destroyDebugUtilsMessengerEXT(debugMessenger, nullptr, dld));
		instance.destroy();
	}

	vk::Instance& get_vulkan_instance() { return instance; }
	vk::SurfaceKHR& get_vulkan_surface() { return surface; }
	SDL_Window* get_window() { return pWindow; }

private:
	void init_sdl_window(int32_t width, int32_t height) {
		// Create an SDL window that supports Vulkan rendering.
		if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) VMI_SDL_ERR();
		pWindow = SDL_CreateWindow(WND_NAME.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
			width, height, 
			SDL_WINDOW_VULKAN);
		if (pWindow == NULL) VMI_SDL_ERR();
	}
	void create_vulkan_instance() {
		std::string spacing = "    ";
		VMI_LOG(spacing << "Vulkan API version: 1.1");


		// Look for all the available extensions
		std::vector<vk::ExtensionProperties> availableExtensions = vk::enumerateInstanceExtensionProperties();
		if (false) {
			VMI_LOG("Available instance extensions:");
			for (const auto& extension : availableExtensions) VMI_LOG(extension.extensionName);
			VMI_LOG("");
		}


		// Get WSI extensions from SDL
		uint32_t nExtensions;
		if (!SDL_Vulkan_GetInstanceExtensions(pWindow, &nExtensions, NULL)) VMI_SDL_ERR();
		std::vector<const char*> extensions(nExtensions);
		if (!SDL_Vulkan_GetInstanceExtensions(pWindow, &nExtensions, extensions.data())) VMI_SDL_ERR();

		// Debug Logging:
		DEBUG_ONLY(vk::DebugUtilsMessengerCreateInfoEXT messengerInfo = Logging::SetupDebugMessenger(extensions));

		// Log output:
		VMI_LOG(spacing << "Required instance extensions:");
		for (const auto& extension : extensions) VMI_LOG(spacing << "- " << extension);

		// Create app info
		vk::ApplicationInfo appInfo = vk::ApplicationInfo()
			.setPApplicationName(WND_NAME.c_str())
			.setApplicationVersion(VK_MAKE_API_VERSION(0, 0, 1, 0))
			.setPEngineName("Vermillion")
			.setEngineVersion(VK_MAKE_API_VERSION(0, 0, 1, 0))
			.setApiVersion(apiVersion);

		// Use validation layer on debug
		std::vector<const char*> layers;
		DEBUG_ONLY(layers.push_back("VK_LAYER_KHRONOS_validation"));

		// Create instance info
		vk::InstanceCreateInfo createInfo = vk::InstanceCreateInfo()
			.setFlags(vk::InstanceCreateFlags())
			.setPApplicationInfo(&appInfo)
			// Extensions
			.setEnabledExtensionCount(static_cast<uint32_t>(extensions.size()))
			.setPpEnabledExtensionNames(extensions.data())
			// Layers
			.setEnabledLayerCount(static_cast<uint32_t>(layers.size()))
			.setPpEnabledLayerNames(layers.data())
			DEBUG_ONLY(.setPNext(&messengerInfo));

		// Finally, create actual instance
		instance = vk::createInstance(createInfo);

		DEBUG_ONLY(dld.init(instance, vkGetInstanceProcAddr));
		DEBUG_ONLY(debugMessenger = instance.createDebugUtilsMessengerEXT(messengerInfo, nullptr, dld));
	}
	void create_vulkan_surface() {
		// Create a Vulkan surface for rendering
		VkSurfaceKHR c_surface;
		if (!SDL_Vulkan_CreateSurface(pWindow, static_cast<VkInstance>(instance), &c_surface)) VMI_SDL_ERR();
		surface = vk::SurfaceKHR(c_surface);
	}

private:
	SDL_Window* pWindow = nullptr;
	vk::Instance instance;
	vk::SurfaceKHR surface;
	DEBUG_ONLY(vk::DispatchLoaderDynamic dld);
	DEBUG_ONLY(vk::DebugUtilsMessengerEXT debugMessenger);

	// lazy constants (settings?)
	const std::string WND_NAME = "Light Field Disparity";
	const uint32_t apiVersion = VK_API_VERSION_1_3;
};