#pragma once

#include "vk_mem_alloc.hpp"
#include "swapchain_wrapper.hpp"
#include "image_wrapper.hpp"
#include "pipelines/swapchain_write.hpp"
#include "pipelines/disparity_compute.hpp"
#include "imgui_wrapper.hpp"


class Renderer 
{
public:
    void init(DeviceWrapper& device, Window& window) {
		VMI_LOG("[Initializing] Renderer...");
		create_vma_allocator(device, window);
		create_command_pools(device);
		create_descriptor_pools(device);

		swapchain.init(device, window);
		create_pipelines(device);
		imgui.init(device, swapchain, window, swapchainWrite);
	}
	void destroy(DeviceWrapper& device)
	{
		destroy_pipelines(device);
		swapchain.destroy(device);

		device.logicalDevice.destroyCommandPool(transientCommandPool);
		device.logicalDevice.destroyCommandPool(transferCommandPool);
		device.logicalDevice.destroyDescriptorPool(descPool);

		imgui.destroy(device);
		allocator.destroy();
	}

public:
	void render(DeviceWrapper& device) {
		uint32_t iSwapchainImage = swapchain.acquire_next_image(device.logicalDevice);
		vk::CommandBuffer commandBuffer = swapchain.record_commands(device, iSwapchainImage);

		disparityImage.transition_layout(commandBuffer, vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eGeneral);
		disparityCompute.execute(commandBuffer);
		disparityImage.transition_layout(commandBuffer, vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal);
		swapchainWrite.execute(commandBuffer, iSwapchainImage);

		swapchain.present(device, iSwapchainImage);
	}

private:
	void create_vma_allocator(DeviceWrapper& device, Window& window) {
		vk::DynamicLoader dl;
		vma::VulkanFunctions vulkanFunctions = vma::VulkanFunctions()
			.setVkGetDeviceProcAddr(dl.getProcAddress<PFN_vkGetDeviceProcAddr>("vkGetDeviceProcAddr"))
			.setVkGetInstanceProcAddr(dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));
		vma::AllocatorCreateInfo info = vma::AllocatorCreateInfo()
			.setPhysicalDevice(device.physicalDevice)
			.setPVulkanFunctions(&vulkanFunctions)
			.setDevice(device.logicalDevice)
			.setInstance(window.get_vulkan_instance())
			.setVulkanApiVersion(VK_API_VERSION)
			.setFlags(vma::AllocatorCreateFlagBits::eKhrDedicatedAllocation);

		allocator = vma::createAllocator(info);
	}
	void create_command_pools(DeviceWrapper& device) {
		vk::CommandPoolCreateInfo commandPoolInfo = vk::CommandPoolCreateInfo()
			.setQueueFamilyIndex(device.iGraphicsQueue)
			.setFlags(vk::CommandPoolCreateFlagBits::eTransient);
		transientCommandPool = device.logicalDevice.createCommandPool(commandPoolInfo);

		commandPoolInfo.setQueueFamilyIndex(device.iTransferQueue);
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

	void create_pipelines(DeviceWrapper& device) {
		vk::ImageUsageFlags usage;

		usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
		lightFieldImage.init(device, allocator, vk::Extent3D(swapchain.get_extent(), 9), usage);
		std::vector<int> indices = { 38, 48, 57, 40, 49, 58, 41, 50, 59 };
		lightFieldImage.load3D(device, allocator, transferCommandPool, "benchmark/training/cotton/", "input_Cam", indices);
		lightFieldImage.transition_layout(device, transferCommandPool, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

		usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment;
		disparityImage.init(device, allocator, vk::Extent3D(swapchain.get_extent(), 1), usage);
		disparityImage.transition_layout(device, transferCommandPool, vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal);

		disparityCompute.init(device, descPool, lightFieldImage, disparityImage);
		swapchainWrite.init(device, swapchain, descPool, disparityImage);
	}
	void destroy_pipelines(DeviceWrapper& device) {
		lightFieldImage.destroy(device, allocator);
		disparityImage.destroy(device, allocator);
		
		disparityCompute.destroy(device);
		swapchainWrite.destroy(device);
	}

private:
	vma::Allocator allocator;
	SwapchainWrapper swapchain;

	DisparityCompute disparityCompute;
	SwapchainWrite swapchainWrite;
	ImguiWrapper imgui;

	ImageWrapper lightFieldImage = { vk::Format::eR8G8B8A8Unorm };
	ImageWrapper disparityImage = { vk::Format::eR32G32B32A32Uint };

	vk::CommandPool transientCommandPool;
	vk::CommandPool transferCommandPool;
	vk::DescriptorPool descPool;
};