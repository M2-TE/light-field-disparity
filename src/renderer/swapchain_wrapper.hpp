#pragma once

class SwapchainWrapper
{
public:
	SwapchainWrapper() = default;
	~SwapchainWrapper() = default;

	void init(DeviceWrapper& deviceWrapper, Window& window)
	{
		choose_surface_format(deviceWrapper);
		choose_present_mode(deviceWrapper);
		choose_extent(deviceWrapper, window);

		create_swapchain(deviceWrapper, window);
		create_images(deviceWrapper);
		create_image_views(deviceWrapper);
	}
	void destroy(vk::Device device)
	{
		device.destroySwapchainKHR(swapchain);

		for (size_t i = 0; i < images.size(); i++) {
			device.destroyImageView(imageViews[i]);
		}
	}

private:
	void choose_surface_format(DeviceWrapper& deviceWrapper)
	{
		const auto& availableFormats = deviceWrapper.formats;
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == targetFormat && availableFormat.colorSpace == targetColorSpace) {
				surfaceFormat = availableFormat;
				return;
			}
		}

		// settle with first format if the requested one isnt available
		VMI_WARN("Matching format not found. Falling back to backup surface format.");
		surfaceFormat = availableFormats[0];
	}
	void choose_present_mode(DeviceWrapper& deviceWrapper)
	{
		const auto& availablePresentModes = deviceWrapper.presentModes;
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == targetPresentMode) {
				presentMode = availablePresentMode;
				return;
			}
		}

		// settle with fifo present mode should the requested one not be available
		presentMode = vk::PresentModeKHR::eFifo;
	}
	void choose_extent(DeviceWrapper& deviceWrapper, Window& window)
	{
		auto& capabilities = deviceWrapper.capabilities;

		int width, height;
		SDL_Vulkan_GetDrawableSize(window.get_window(), &width, &height);

		extent = vk::Extent2D()
			.setWidth(width)
			.setHeight(height);
	}
	void create_swapchain(DeviceWrapper& deviceWrapper, Window& window)
	{
		if (deviceWrapper.capabilities.minImageCount > nTargetSwapchainImages) nImages = deviceWrapper.capabilities.minImageCount;
		else nImages = nTargetSwapchainImages;

		VMI_LOG("    Selected swapchain formatting:");
		VMI_LOG("    - Format: " << (uint32_t)surfaceFormat.format);
		VMI_LOG("    - Color space: " << (uint32_t)surfaceFormat.colorSpace);
		vk::SwapchainCreateInfoKHR swapchainInfo = vk::SwapchainCreateInfoKHR()
			// image settings
			.setMinImageCount(nImages)
			.setImageFormat(surfaceFormat.format)
			.setImageColorSpace(surfaceFormat.colorSpace)
			.setImageExtent(extent)
			.setImageArrayLayers(1)
			.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)

			// when both queues are the same, create exclusive access, else create concurrent access
			.setImageSharingMode(vk::SharingMode::eExclusive)
			.setQueueFamilyIndexCount(0)
			.setPQueueFamilyIndices(nullptr)

			// both of these should be the same
			.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
			.setPreTransform(deviceWrapper.capabilities.currentTransform)

			// misc
			.setSurface(window.get_vulkan_surface())
			.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
			.setPresentMode(presentMode)
			.setClipped(VK_TRUE)
			.setOldSwapchain(nullptr); // pointer to old swapchain on resize

		// finally, create swapchain
		swapchain = deviceWrapper.logicalDevice.createSwapchainKHR(swapchainInfo);
	}

	void create_images(DeviceWrapper& deviceWrapper)
	{
		images = deviceWrapper.logicalDevice.getSwapchainImagesKHR(swapchain);
	}
	void create_image_views(DeviceWrapper& deviceWrapper)
	{
		vk::ComponentMapping mapping = vk::ComponentMapping()
			.setR(vk::ComponentSwizzle::eIdentity)
			.setG(vk::ComponentSwizzle::eIdentity)
			.setB(vk::ComponentSwizzle::eIdentity)
			.setA(vk::ComponentSwizzle::eIdentity);

		vk::ImageSubresourceRange subrange = vk::ImageSubresourceRange()
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setLevelCount(1).setBaseMipLevel(0)
			.setLayerCount(1).setBaseArrayLayer(0);

		imageViews.resize(images.size());
		for (size_t i = 0; i < imageViews.size(); i++) {
			vk::ImageViewCreateInfo imageInfo = vk::ImageViewCreateInfo()
				.setImage(images[i])
				.setViewType(vk::ImageViewType::e2D)
				.setFormat(surfaceFormat.format)
				.setComponents(mapping)
				.setSubresourceRange(subrange);

			imageViews[i] = deviceWrapper.logicalDevice.createImageView(imageInfo);
		}
	}

private:
	static constexpr vk::Format targetFormat = vk::Format::eB8G8R8A8Srgb;
	static constexpr vk::ColorSpaceKHR targetColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
	static constexpr vk::PresentModeKHR targetPresentMode = vk::PresentModeKHR::eFifo; // vsync
	//static constexpr vk::PresentModeKHR targetPresentMode = vk::PresentModeKHR::eFifoRelaxed; // vsync?
	//static constexpr vk::PresentModeKHR targetPresentMode = vk::PresentModeKHR::eImmediate; // no vsync, tearing very likely
	//static constexpr vk::PresentModeKHR targetPresentMode = vk::PresentModeKHR::eMailbox; // better than immediate, if available

public:
	vk::SwapchainKHR swapchain;
	vk::SurfaceFormatKHR surfaceFormat;
	vk::PresentModeKHR presentMode;
	vk::Extent2D extent;

	uint32_t nImages;
	std::vector<vk::Image> images;
	std::vector<vk::ImageView> imageViews;

private:
	static constexpr uint32_t nTargetSwapchainImages = 2; // max frames in flight (minimum for swapchain)
};