class Renderer 
{
public:
    void init(DeviceWrapper& deviceWrapper, Window& window) {
		VMI_LOG("[Initializing] Renderer...");
		create_vma_allocator(deviceWrapper, window);
		// create_descriptor_pools(deviceWrapper);
		// create_command_pools(deviceWrapper);

		// create_KHR(deviceWrapper, window, lightfieldDir);
		// syncFrames.set_size(swapchainWrapper.nImages).init(deviceWrapper);

		// imguiWrapper.init(deviceWrapper, window, swapchainWriteRenderpass.get_render_pass(), syncFrames);
	}
	void destroy(vk::Device device)
	{
		// destroy_KHR(deviceWrapper);

		// device.destroyCommandPool(transientCommandPool);
		// device.destroyDescriptorPool(descPool);

		// syncFrames.destroy(deviceWrapper);

		// imguiWrapper.destroy(deviceWrapper);
		// ImGui_ImplVulkan_Shutdown();

		// deallocate_entities(deviceWrapper, reg);
		allocator.destroy();
	}

public:

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

private:
	vma::Allocator allocator;
};