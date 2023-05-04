#pragma once

#include "window/window.hpp"
#include "sync_frame.hpp"

class SwapchainWrapper
{
public:
	SwapchainWrapper() = default;
	~SwapchainWrapper() = default;

	void init(DeviceWrapper& device, Window& window) {
		choose_surface_format(device);
		choose_present_mode(device);
		choose_extent(device, window);

		create_swapchain(device, window);
		create_images(device);
		create_image_views(device);
		create_sync_frames(device);
	}
	void destroy(DeviceWrapper& device) {
		device.logicalDevice.destroySwapchainKHR(swapchain);

		for (size_t i = 0; i < images.size(); i++) {
			device.logicalDevice.destroyImageView(imageViews[i]);
			syncFrames[i].destroy(device);
		}
	}

	inline SyncFrame& get_sync_frame() { return syncFrames[curSyncFrame]; }
	inline vk::ImageView& get_image_view(uint32_t i) { return imageViews[i]; }
	inline uint32_t get_image_count() { return images.size(); }
	inline vk::Extent2D get_extent() { return extent; }
	inline vk::SurfaceFormatKHR get_surface_format() { return surfaceFormat; }

	uint32_t acquire_next_image(vk::Device device) {

		// get next frame of sync array
		curSyncFrame = (curSyncFrame + 1) % syncFrames.size();
		SyncFrame& syncFrame = syncFrames[curSyncFrame];

		// Acquire image
		vk::ResultValue imgResult = device.acquireNextImageKHR(swapchain, UINT64_MAX, syncFrame.imageAvailable);
		switch (imgResult.result) {
			case vk::Result::eSuccess: break;
			case vk::Result::eSuboptimalKHR: VMI_LOG("Suboptimal image acquisition."); break;
			case vk::Result::eErrorOutOfDateKHR: VMI_ERR("Swapchain (Image Acquisition): KHR out of date."); break;
			default: assert(false);
		}

		return imgResult.value;
	}
	vk::CommandBuffer record_commands(DeviceWrapper& device, uint32_t iSwapchainImage) {

		// wait for fence of fetched frame before rendering to it
		SyncFrame& syncFrame = syncFrames[curSyncFrame];
		vk::Result result = device.logicalDevice.waitForFences(syncFrame.commandBufferFence, VK_TRUE, UINT64_MAX);
		if (result != vk::Result::eSuccess) assert(false);
		
		// and reset it
		device.logicalDevice.resetFences(syncFrame.commandBufferFence);

		// reset command pool and then record into it (using command buffer)
		device.logicalDevice.resetCommandPool(syncFrame.commandPool);

		// setting up command buffer
		vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo()
			.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
			.setPInheritanceInfo(nullptr);
		syncFrame.commandBuffer.begin(beginInfo);

		return syncFrame.commandBuffer;
	}
	void present(DeviceWrapper& device, uint32_t iFrame) {
		SyncFrame& syncFrame = syncFrames[curSyncFrame];

		// finalize command buffer
		syncFrame.commandBuffer.end();

		// Render (submit commands)
		vk::PipelineStageFlags waitStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		vk::SubmitInfo submitInfo = vk::SubmitInfo()
			.setPWaitDstStageMask(&waitStages)
			// semaphores
			.setWaitSemaphoreCount(1).setPWaitSemaphores(&syncFrame.imageAvailable)
			.setSignalSemaphoreCount(1).setPSignalSemaphores(&syncFrame.renderFinished)
			// command buffers
			.setCommandBufferCount(1).setPCommandBuffers(&syncFrame.commandBuffer);

		device.graphicsQueue.submit(submitInfo, syncFrame.commandBufferFence);

		// Present
		vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
			.setPImageIndices(&iFrame)
			// semaphores
			.setWaitSemaphoreCount(1).setPWaitSemaphores(&syncFrame.renderFinished)
			// swapchains
			.setSwapchainCount(1).setPSwapchains(&swapchain);

		vk::Result result = device.graphicsQueue.presentKHR(&presentInfo);
		switch (result) {
			case vk::Result::eSuccess: break;
			case vk::Result::eErrorOutOfDateKHR: VMI_ERR("Swapchain (Present): KHR out of date."); break;
			default: assert(false);
		}
	}

private:
	void choose_surface_format(DeviceWrapper& device) {
		const auto& availableFormats = device.formats;
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
	void choose_present_mode(DeviceWrapper& device) {
		const auto& availablePresentModes = device.presentModes;
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == targetPresentMode) {
				presentMode = availablePresentMode;
				return;
			}
		}

		// settle with fifo present mode should the requested one not be available
		VMI_LOG("Requested present mode is not available");
		presentMode = vk::PresentModeKHR::eFifo;
	}
	void choose_extent(DeviceWrapper& device, Window& window) {
		auto& capabilities = device.capabilities;

		int width, height;
		SDL_Vulkan_GetDrawableSize(window.get_window(), &width, &height);

		extent = vk::Extent2D()
			.setWidth(width)
			.setHeight(height);
	}
	void create_swapchain(DeviceWrapper& device, Window& window) {
		if (device.capabilities.minImageCount > nTargetSwapchainImages) nImages = device.capabilities.minImageCount;
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
			.setPreTransform(device.capabilities.currentTransform)

			// misc
			.setSurface(window.get_vulkan_surface())
			.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
			.setPresentMode(presentMode)
			.setClipped(VK_TRUE)
			.setOldSwapchain(nullptr); // pointer to old swapchain on resize

		// finally, create swapchain
		swapchain = device.logicalDevice.createSwapchainKHR(swapchainInfo);
	}

	void create_images(DeviceWrapper& device) {
		images = device.logicalDevice.getSwapchainImagesKHR(swapchain);
	}
	void create_image_views(DeviceWrapper& device) {
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

			imageViews[i] = device.logicalDevice.createImageView(imageInfo);
		}
	}
	void create_sync_frames(DeviceWrapper& device) {
		syncFrames.resize(images.size());
		for (int i = 0; i < images.size(); i++) {
			syncFrames[i].init(device);
		}
	}

private:
	// some settings
	static constexpr uint32_t nTargetSwapchainImages = 2; // targeted max frames in flight
	static constexpr vk::Format targetFormat = vk::Format::eB8G8R8A8Srgb;
	static constexpr vk::ColorSpaceKHR targetColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
	static constexpr vk::PresentModeKHR targetPresentMode = vk::PresentModeKHR::eFifo; // vsync
	//static constexpr vk::PresentModeKHR targetPresentMode = vk::PresentModeKHR::eFifoRelaxed; // vsync?
	//static constexpr vk::PresentModeKHR targetPresentMode = vk::PresentModeKHR::eImmediate; // no vsync, tearing very likely
	//static constexpr vk::PresentModeKHR targetPresentMode = vk::PresentModeKHR::eMailbox; // better than immediate, if available

private:
	vk::SwapchainKHR swapchain;
	vk::SurfaceFormatKHR surfaceFormat;
	vk::PresentModeKHR presentMode;
	vk::Extent2D extent;

	uint32_t nImages;
	std::vector<vk::Image> images;
	std::vector<vk::ImageView> imageViews;
	std::vector<SyncFrame> syncFrames;
	uint32_t curSyncFrame = 0;
};