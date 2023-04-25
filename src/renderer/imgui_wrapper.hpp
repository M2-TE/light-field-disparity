#pragma once

class ImguiWrapper
{
public:
	ImguiWrapper() = default;
	~ImguiWrapper() = default;

public:
	void init(DeviceWrapper& deviceWrapper, Window& window, vk::RenderPass& renderPass, RingBuffer<SyncFrameData>& syncFrames)
	{
		imgui_create_desc_pool(deviceWrapper);
		imgui_init_vulkan(deviceWrapper, window, renderPass, syncFrames);
		imgui_upload_fonts(deviceWrapper, syncFrames);
	}
	void destroy(DeviceWrapper& deviceWrapper)
	{
		deviceWrapper.logicalDevice.destroyDescriptorPool(descPool);
		ImGui_ImplVulkan_Shutdown();
	}

private:
	void imgui_create_desc_pool(DeviceWrapper& deviceWrapper)
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

		descPool = deviceWrapper.logicalDevice.createDescriptorPool(info);
	}
	void imgui_init_vulkan(DeviceWrapper& deviceWrapper, Window& window, vk::RenderPass& renderPass, RingBuffer<SyncFrameData>& syncFrames)
	{
		struct ImGui_ImplVulkan_InitInfo info = { 0 };
		info.Instance = window.get_vulkan_instance();
		info.PhysicalDevice = deviceWrapper.physicalDevice;
		info.Device = deviceWrapper.logicalDevice;
		info.QueueFamily = deviceWrapper.iQueue;
		info.Queue = deviceWrapper.queue;
		info.PipelineCache = nullptr;
		info.DescriptorPool = descPool;
		info.Subpass = 0;
		info.MinImageCount = syncFrames.get_size();
		info.ImageCount = syncFrames.get_size();
		info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&info, renderPass);
	}
	void imgui_upload_fonts(DeviceWrapper& deviceWrapper, RingBuffer<SyncFrameData>& syncFrames)
	{
		SyncFrameData& syncFrame = syncFrames.get_next();
		vk::CommandBuffer commandBuffer = syncFrame.commandBuffer;
		deviceWrapper.logicalDevice.resetCommandPool(syncFrame.commandPool);

		vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo()
			.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		commandBuffer.begin(beginInfo);

		// upload fonts
		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

		commandBuffer.end();
		vk::SubmitInfo submitInfo = vk::SubmitInfo()
			.setCommandBufferCount(1)
			.setPCommandBuffers(&commandBuffer);
		deviceWrapper.queue.submit(submitInfo);

		deviceWrapper.logicalDevice.waitIdle();
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

private:
	vk::DescriptorPool descPool;
};