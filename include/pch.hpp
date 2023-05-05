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
#define VK_API_VERSION VK_API_VERSION_1_3 // use vulkan 1.3
// -> vulkan 1.1 is sufficient, but a spir-v thing throws some validation warnings (which are annoying)
#include <vulkan/vulkan.hpp>

// SDL
#define SDL_MAIN_HANDLED
#include "SDL3/SDL.h"
#include "SDL3/SDL_vulkan.h"

// ImGui
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>

// Utils
#include "utils/logging.hpp"