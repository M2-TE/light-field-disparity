#include "swapchain_wrapper.hpp"
#include "image_wrapper.hpp"
#include "render_passes/swapchain_write.hpp"
// #include "uniform_buffer.hpp"
// #include "imgui_wrapper.hpp"


class Renderer 
{
public:
    void init(DeviceWrapper& device, Window& window) {
		VMI_LOG("[Initializing] Renderer...");
		create_vma_allocator(device, window);
		create_command_pools(device);
		create_descriptor_pools(device);

		swapchain.init(device, window);
		create_render_pipelines(device);

		//imguiWrapper.init(device, window, swapchainWriteRenderpass.get_render_pass(), syncFrames);
	}
	void destroy(DeviceWrapper& device)
	{
		destroy_render_pipelines(device);
		swapchain.destroy(device);

		device.logicalDevice.destroyCommandPool(transientCommandPool);
		device.logicalDevice.destroyCommandPool(transferCommandPool);
		device.logicalDevice.destroyDescriptorPool(descPool);

		// imguiWrapper.destroy(device);
		allocator.destroy();
	}

public:
	void render(DeviceWrapper& device) {
		uint32_t iSwapchainImage = swapchain.acquire_next_image(device.logicalDevice);
		vk::CommandBuffer commandBuffer = swapchain.record_commands(device, iSwapchainImage);

		// direct write to swapchain image
		swapchainWrite.execute(commandBuffer, iSwapchainImage);

		swapchain.present(device, iSwapchainImage);
	}

private:
	void create_vma_allocator(DeviceWrapper& device, Window& window) {
		vma::AllocatorCreateInfo info = vma::AllocatorCreateInfo()
			.setPhysicalDevice(device.physicalDevice)
			.setDevice(device.logicalDevice)
			.setInstance(window.get_vulkan_instance())
			.setVulkanApiVersion(VK_API_VERSION_1_1)
			.setFlags(vma::AllocatorCreateFlagBits::eKhrDedicatedAllocation);

		allocator = vma::createAllocator(info);
	}
	void create_command_pools(DeviceWrapper& device) {
		vk::CommandPoolCreateInfo commandPoolInfo = vk::CommandPoolCreateInfo()
			.setQueueFamilyIndex(device.iQueue)
			.setFlags(vk::CommandPoolCreateFlagBits::eTransient);
		transientCommandPool = device.logicalDevice.createCommandPool(commandPoolInfo);

		commandPoolInfo.setQueueFamilyIndex(device.iQueue);
		transferCommandPool = device.logicalDevice.createCommandPool(commandPoolInfo);
	}
	void create_descriptor_pools(DeviceWrapper& device) {
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
		descPool = device.logicalDevice.createDescriptorPool(info);
	}

	void create_render_pipelines(DeviceWrapper& device) {
		disparityImage.init(device, swapchain, allocator);
		swapchainWrite.init(device, swapchain, descPool, disparityImage);
	}
	void destroy_render_pipelines(DeviceWrapper& device) {
		swapchainWrite.destroy(device);
		disparityImage.destroy(device, allocator);
	}

private:
	vma::Allocator allocator;
	SwapchainWrapper swapchain;
	// ImguiWrapper imguiWrapper;

	SwapchainWrite swapchainWrite;
	ImageWrapper disparityImage = { vk::Format::eR32Sfloat };


	vk::CommandPool transientCommandPool;
	vk::CommandPool transferCommandPool;
	vk::DescriptorPool descPool;
};