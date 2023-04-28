#include <chrono>
#include <iostream>
#include <vector>
#include <queue>
#include <optional>
#include <set>
#include <cstdint>
#include <algorithm>
#include <fstream>
#include <filesystem>

// load vulkan functions dynamically
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VK_API_VERSION VK_API_VERSION_1_1 // use vulkan 1.1
#include <vulkan/vulkan.hpp>

// Vulkan Memory Allocator with hpp bindings
// -> stb-style lib implemented in pch.cpp

// Tell SDL not to mess with main()
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>

// ImGui
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_vulkan.h>

// Utils
#include "utils/logging.hpp"