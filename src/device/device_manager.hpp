#pragma once

#include "device_wrapper.hpp"

class DeviceManager
{
public:
	void init(vk::Instance& instance, vk::SurfaceKHR& surface)
	{
		VMI_LOG("[Initializing] Device manager...");
		std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
		if (physicalDevices.empty()) VMI_ERR("Failed to find GPUs with Vulkan support.");
		devices.reserve(physicalDevices.size());
		for (vk::PhysicalDevice& physicalDevice : physicalDevices) devices.emplace_back(physicalDevice, surface);

		pick_best_physical_device(surface);
		get_device_wrapper().create_logical_device();
	}
	void destroy()
	{
		get_device_wrapper().destroy_logical_device();
	}

	inline vk::PhysicalDevice& get_physical_device() { return devices[iCurrentDevice].physicalDevice; }
	inline vk::Device& get_logical_device() { return devices[iCurrentDevice].logicalDevice; }
	inline DeviceWrapper& get_device_wrapper() { return devices[iCurrentDevice]; }

private:
	void pick_best_physical_device(vk::SurfaceKHR& surface)
	{
		int highscore = -1;
		for (int i = 0; i < devices.size(); i++)
		{
			int score = devices[i].get_device_score();
			if (score > highscore) {
				highscore = score;
				iCurrentDevice = i;
			}
		}

		if (highscore == -1) VMI_ERR("Failed to find a suitable GPU.");
	}

private:
	std::vector<DeviceWrapper> devices;
	int iCurrentDevice = -1;
};