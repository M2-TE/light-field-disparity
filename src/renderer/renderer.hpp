#include "swapchain_wrapper.hpp"
// #include "imgui_wrapper.hpp"

class Renderer 
{
public:
    void init(DeviceWrapper& deviceWrapper, Window& window) {
		VMI_LOG("[Initializing] Renderer...");
		create_vma_allocator(deviceWrapper, window);
		create_command_pools(deviceWrapper);
		create_descriptor_pools(deviceWrapper);

		swapchainWrapper.init(deviceWrapper, window);
		create_render_pipelines();

		// syncFrames.set_size(swapchainWrapper.nImages).init(deviceWrapper);

		// imguiWrapper.init(deviceWrapper, window, swapchainWriteRenderpass.get_render_pass(), syncFrames);
	}
	void destroy(vk::Device device)
	{
		destroy_render_pipelines();
		swapchainWrapper.destroy(device);

		device.destroyCommandPool(transientCommandPool);
		device.destroyCommandPool(transferCommandPool);
		device.destroyDescriptorPool(descPool);

		// syncFrames.destroy(deviceWrapper);

		// imguiWrapper.destroy(deviceWrapper);
		// ImGui_ImplVulkan_Shutdown();

		// deallocate_entities(deviceWrapper, reg);
		allocator.destroy();
	}

public:
	void render() {
		// TODO
	}

private:
	void create_vma_allocator(DeviceWrapper& deviceWrapper, Window& window) {
		vma::AllocatorCreateInfo info = vma::AllocatorCreateInfo()
			.setPhysicalDevice(deviceWrapper.physicalDevice)
			.setDevice(deviceWrapper.logicalDevice)
			.setInstance(window.get_vulkan_instance())
			.setVulkanApiVersion(VK_API_VERSION_1_1)
			.setFlags(vma::AllocatorCreateFlagBits::eKhrDedicatedAllocation);

		allocator = vma::createAllocator(info);
	}
	void create_command_pools(DeviceWrapper& deviceWrapper) {
		vk::CommandPoolCreateInfo commandPoolInfo = vk::CommandPoolCreateInfo()
			.setQueueFamilyIndex(deviceWrapper.iQueue)
			.setFlags(vk::CommandPoolCreateFlagBits::eTransient);
		transientCommandPool = deviceWrapper.logicalDevice.createCommandPool(commandPoolInfo);

		commandPoolInfo.setQueueFamilyIndex(deviceWrapper.iQueue);
		transferCommandPool = deviceWrapper.logicalDevice.createCommandPool(commandPoolInfo);
	}
	void create_descriptor_pools(DeviceWrapper& deviceWrapper) {
		static constexpr uint32_t poolSize = 1000;
		std::array<vk::DescriptorPoolSize, 1>  poolSizes = {
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, poolSize)
			// TODO: other stuff this pool will need
		};
		vk::DescriptorPoolCreateFlags flags;

		vk::DescriptorPoolCreateInfo info = vk::DescriptorPoolCreateInfo()
			.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
			.setMaxSets(poolSize * (uint32_t)poolSizes.size())
			.setPoolSizeCount((uint32_t)poolSizes.size())
			.setPPoolSizes(poolSizes.data());
		descPool = deviceWrapper.logicalDevice.createDescriptorPool(info);
	}

	void create_render_pipelines() {
		// TODO
	}
	void destroy_render_pipelines() {
		// TODO
	}

private:
	vma::Allocator allocator;
	SwapchainWrapper swapchainWrapper;
	// ImguiWrapper imguiWrapper;

	// Lightfield lightfield;
	// ForwardRenderpass forwardRenderpass;
	// GradientsRenderpass gradientsRenderpass;
	// DisparityRenderpass disparityRenderpass;
	// SwapchainWrite swapchainWriteRenderpass;

	// RingBuffer<SyncFrameData> syncFrames;
	vk::CommandPool transientCommandPool;
	vk::CommandPool transferCommandPool;
	vk::DescriptorPool descPool;
};