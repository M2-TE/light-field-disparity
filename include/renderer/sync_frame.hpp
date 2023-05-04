#pragma once

#include "device/device_wrapper.hpp"

class SyncFrame
{
public:
	void init(DeviceWrapper& device)
	{
		create_semaphores(device);
		create_fence(device);
		create_command_pools(device);
		create_command_buffer(device);
	}
	void destroy(DeviceWrapper& device)
	{
		device.logicalDevice.destroySemaphore(imageAvailable);
		device.logicalDevice.destroySemaphore(renderFinished);
		device.logicalDevice.destroyFence(commandBufferFence);
		device.logicalDevice.destroyCommandPool(commandPool);
	}

private:
	void create_semaphores(DeviceWrapper& device)
	{
		vk::SemaphoreCreateInfo semaphoreInfo = vk::SemaphoreCreateInfo();

		imageAvailable = device.logicalDevice.createSemaphore(semaphoreInfo);
		renderFinished = device.logicalDevice.createSemaphore(semaphoreInfo);
	}
	void create_fence(DeviceWrapper& device)
	{
		vk::FenceCreateInfo fenceInfo = vk::FenceCreateInfo()
			.setFlags(vk::FenceCreateFlagBits::eSignaled);

		commandBufferFence = device.logicalDevice.createFence(fenceInfo);
	}
	void create_command_pools(DeviceWrapper& device)
	{
		vk::CommandPoolCreateInfo commandPoolInfo;

		commandPoolInfo = vk::CommandPoolCreateInfo()
			.setQueueFamilyIndex(device.iGraphicsQueue);
		commandPool = device.logicalDevice.createCommandPool(commandPoolInfo);

	}
	void create_command_buffer(DeviceWrapper& device)
	{
		vk::CommandBufferAllocateInfo commandBufferInfo = vk::CommandBufferAllocateInfo()
			.setCommandPool(commandPool)
			.setLevel(vk::CommandBufferLevel::ePrimary) // secondary are used by primary command buffers for e.g. common operations
			.setCommandBufferCount(1);
		commandBuffer = device.logicalDevice.allocateCommandBuffers(commandBufferInfo)[0];
	}

public:
	vk::Semaphore imageAvailable;
	vk::Semaphore renderFinished;
	vk::Fence commandBufferFence;

	vk::CommandPool commandPool;
	vk::CommandBuffer commandBuffer;
};