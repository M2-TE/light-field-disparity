#pragma once

class DeviceWrapper
{
public:
	DeviceWrapper(vk::PhysicalDevice& physicalDevice, vk::SurfaceKHR& surface) :
		physicalDevice(physicalDevice), iGraphicsQueue(UINT32_MAX), iComputeQueue(UINT32_MAX), iTransferQueue(UINT32_MAX)
	{
		physicalDevice.getProperties(&deviceProperties);
		physicalDevice.getFeatures(&deviceFeatures);
		physicalDevice.getMemoryProperties(&deviceMemProperties);

		query_swapchain_support_details(surface);
		assign_queue_family_index(surface);
	}

	int32_t get_device_score() {
		int32_t deviceScore = 0;

		// Discrete GPUs have a significant performance advantage
		if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) deviceScore += 1000;

		// Maximum possible size of textures affects graphics quality
		deviceScore += deviceProperties.limits.maxImageDimension2D;

		if (iGraphicsQueue == UINT32_MAX) return -1; // check for valid queue index
		else if (formats.empty() || presentModes.empty()) return -1;
		else return deviceScore;
	}
	void create_logical_device() {
		std::string spacing = "    ";
		VMI_LOG(spacing << "Required device extensions:");
		std::vector<const char*> requiredDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		for (const auto& extension : requiredDeviceExtensions) VMI_LOG(spacing << "- " << extension);
		VMI_LOG("");

		VMI_LOG(spacing << "Optional device extensions:");
		std::vector<const char*> optionalDeviceExtensions = {
			VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
			VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME
		};
		for (const auto& extension : optionalDeviceExtensions) VMI_LOG(spacing << "- " << extension);
		VMI_LOG("");

		VMI_LOG(spacing << "Available device extensions:");
		std::vector<vk::ExtensionProperties> availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();
		for (size_t i = 0; i < optionalDeviceExtensions.size(); i++) {
			std::string optionalExt = optionalDeviceExtensions[i];
			for (const auto& extension : availableExtensions) {
				if (optionalExt.find(extension.extensionName, 0, optionalExt.size()) < 1) {
					requiredDeviceExtensions.push_back(extension.extensionName);
				}
			}
		}
		for (const auto& extension : requiredDeviceExtensions) VMI_LOG(spacing << "- " << extension);

		vk::PhysicalDeviceFeatures deviceFeatures = vk::PhysicalDeviceFeatures()
			.setShaderFloat64(true)
			// .setShaderInt64(true)
			.setShaderStorageImageReadWithoutFormat(true);


		std::vector<vk::DeviceQueueCreateInfo> queueInfos;
		queueInfos.reserve(3);
		float priority = 1.0f;
		// queueInfos.push_back(vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags(0), iGraphicsQueue, 1, &priority));
		queueInfos.emplace_back(vk::DeviceQueueCreateFlags(0), iGraphicsQueue, 1, &priority);

		if (iComputeQueue == iGraphicsQueue) {
			VMI_WARN("No dedicated compute queue family found. Falling back to graphics queue family");
		} else {
			queueInfos.emplace_back(vk::DeviceQueueCreateFlags(0), iComputeQueue, 1, &priority);
		}

		if (iTransferQueue == iGraphicsQueue) {
			VMI_WARN("No dedicated transfer queue family found. Falling back to graphics queue family");
		} else {
			queueInfos.emplace_back(vk::DeviceQueueCreateFlags(0), iTransferQueue, 1, &priority);
		}

		vk::DeviceCreateInfo createInfo = vk::DeviceCreateInfo()
			.setEnabledExtensionCount((uint32_t)requiredDeviceExtensions.size()).setPpEnabledExtensionNames(requiredDeviceExtensions.data())
			.setQueueCreateInfos(queueInfos)
			.setPEnabledFeatures(&deviceFeatures);

		// Create logical device
		logicalDevice = physicalDevice.createDevice(createInfo);

		// get actual handles for queues
		graphicsQueue = logicalDevice.getQueue(iGraphicsQueue, 0);
		computeQueue = iComputeQueue == iGraphicsQueue ? graphicsQueue : logicalDevice.getQueue(iComputeQueue, 0);
		transferQueue = iTransferQueue == iGraphicsQueue ? graphicsQueue : logicalDevice.getQueue(iTransferQueue, 0);

		VMI_LOG("[Initializing] Device-specific vulkan functions...");
		VULKAN_HPP_DEFAULT_DISPATCHER.init(logicalDevice);
	}
	void destroy_logical_device() { logicalDevice.destroy(); }

private:
	void assign_queue_family_index(vk::SurfaceKHR& surface) {
		// find a queue family that supports both graphics and presentation
		std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();

#ifdef NDEBUG_DISABLED // disabled debug output for now
		// query all queue families
		for (int i = 0; i < queueFamilies.size(); i++) {
			std::ostringstream oss;
			oss << "Queue family " << i;
			oss << " supports " << queueFamilies[i].queueCount << " queues of type:";
			if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) oss << " graphics |";
			if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eTransfer) oss << " transfer |";
			if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute) oss << " compute |";
			if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eProtected) oss << " protected |";
			if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eSparseBinding) oss << " sparse binding |";
			oss.seekp(-2, oss.cur);
			oss << ". ";
			VMI_LOG(oss.str());
		}
		VMI_LOG("");
#endif

		// get optimal graphics queue
		for (int i = 0; i < queueFamilies.size(); i++) {

			if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics &&
				physicalDevice.getSurfaceSupportKHR(i, surface)) {

				iGraphicsQueue = i;
				break;
			}
		}
		// get optimal compute queue
		for (int i = 0; i < queueFamilies.size(); i++) {

			if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute && 
				!(queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics)) {

				iComputeQueue = i;
				break;
			}
		}
		// get optimal transfer queue
		for (int i = 0; i < queueFamilies.size(); i++) {

			if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eTransfer &&
				!(queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
				!(queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute)) {

				iTransferQueue = i;
				break;
			}
		}

		// if no dedicated queues are not found, use graphics queue
		if (iComputeQueue == UINT32_MAX) iComputeQueue = iGraphicsQueue;
		if (iTransferQueue == UINT32_MAX) iTransferQueue = iGraphicsQueue;
	}
	void query_swapchain_support_details(vk::SurfaceKHR& surface) {
		capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
		formats = physicalDevice.getSurfaceFormatsKHR(surface);
		presentModes = physicalDevice.getSurfacePresentModesKHR(surface);
	}

public:
	vk::PhysicalDevice physicalDevice;
	vk::Device logicalDevice;

	vk::Queue graphicsQueue, computeQueue, transferQueue;
	uint32_t iGraphicsQueue, iComputeQueue, iTransferQueue;

	// some properties of the device
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> presentModes;

	vk::PhysicalDeviceProperties deviceProperties;
	vk::PhysicalDeviceFeatures deviceFeatures;
	vk::PhysicalDeviceMemoryProperties deviceMemProperties;
};