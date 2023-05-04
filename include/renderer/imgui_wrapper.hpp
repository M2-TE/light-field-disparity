#pragma once

class ImguiWrapper
{
public:
	ImguiWrapper() = default;
	~ImguiWrapper() = default;

public:
	void init(DeviceWrapper& device, SwapchainWrapper& swapchain, Window& window, SwapchainWrite& swapchainWrite)
	{
		imgui_create_desc_pool(device);
		imgui_init_vulkan(device, swapchain, window, swapchainWrite);
		imgui_upload_fonts(device, swapchain);
	}
	void destroy(DeviceWrapper& device)
	{
		device.logicalDevice.destroyDescriptorPool(descPool);
		ImGui_ImplVulkan_Shutdown();
	}

private:
	void imgui_create_desc_pool(DeviceWrapper& device)
	{
		uint32_t descCountImgui = 1000;
		std::array<vk::DescriptorPoolSize, 11> poolSizes =
		{
			vk::DescriptorPoolSize(vk::DescriptorType::eSampler, descCountImgui),
			vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, descCountImgui),
			vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, descCountImgui),
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, descCountImgui),
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformTexelBuffer, descCountImgui),
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageTexelBuffer, descCountImgui),
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, descCountImgui),
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, descCountImgui),
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, descCountImgui),
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageBufferDynamic, descCountImgui),
			vk::DescriptorPoolSize(vk::DescriptorType::eInputAttachment, descCountImgui)
		};

		vk::DescriptorPoolCreateInfo info = vk::DescriptorPoolCreateInfo()
			.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
			.setMaxSets(descCountImgui * (uint32_t)poolSizes.size())
			.setPoolSizeCount((uint32_t)poolSizes.size())
			.setPPoolSizes(poolSizes.data());

		descPool = device.logicalDevice.createDescriptorPool(info);
	}
	void imgui_init_vulkan(DeviceWrapper& device, SwapchainWrapper& swapchain, Window& window, SwapchainWrite& swapchainWrite)
	{
		struct ImGui_ImplVulkan_InitInfo info = { 0 };
		info.Instance = window.get_vulkan_instance();
		info.PhysicalDevice = device.physicalDevice;
		info.Device = device.logicalDevice;
		info.QueueFamily = device.iGraphicsQueue;
		info.Queue = device.graphicsQueue;
		info.PipelineCache = nullptr;
		info.DescriptorPool = descPool;
		info.Subpass = 0;
		info.MinImageCount = swapchain.get_image_count();
		info.ImageCount = swapchain.get_image_count();
		info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&info, swapchainWrite.get_render_pass());
	}
	void imgui_upload_fonts(DeviceWrapper& device, SwapchainWrapper& swapchain)
	{
		SyncFrame& syncFrame = swapchain.get_sync_frame();
		vk::CommandBuffer commandBuffer = syncFrame.commandBuffer;
		device.logicalDevice.resetCommandPool(syncFrame.commandPool);

		vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo()
			.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		commandBuffer.begin(beginInfo);

		// upload fonts
		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

		commandBuffer.end();
		vk::SubmitInfo submitInfo = vk::SubmitInfo()
			.setCommandBufferCount(1)
			.setPCommandBuffers(&commandBuffer);
		device.graphicsQueue.submit(submitInfo);

		device.logicalDevice.waitIdle();
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

private:
	vk::DescriptorPool descPool;
};