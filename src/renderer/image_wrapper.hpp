#pragma once

class ImageWrapper 
{
public:
    ImageWrapper(vk::Format colorFormat = vk::Format::eR8G8B8A8Srgb) : colorFormat(colorFormat) {}
    ~ImageWrapper() = default;

public:
    void init(DeviceWrapper& device, vma::Allocator allocator, vk::Extent3D extent, vk::ImageUsageFlags usage) {
        this->extent = extent;
        create_image(allocator, usage);
        create_image_view(device);
    }
    void destroy(DeviceWrapper& device, vma::Allocator allocator) {
        allocator.destroyImage(image, alloc);
        device.logicalDevice.destroyImageView(imageView);
    }
    
    void transition_layout(DeviceWrapper& device, vk::CommandPool commandPool, vk::ImageLayout from, vk::ImageLayout to) {
        vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo()
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandPool(commandPool)
            .setCommandBufferCount(1);

        vk::CommandBuffer commandBuffer;
        vk::Result res = device.logicalDevice.allocateCommandBuffers(&allocInfo, &commandBuffer);
        if (res != vk::Result::eSuccess) VMI_ERR("Failed to allocate command buffer");

        vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		commandBuffer.begin(beginInfo);
        transition_layout(commandBuffer, from, to);
        commandBuffer.end();

        vk::SubmitInfo submitInfo = vk::SubmitInfo().setCommandBuffers(commandBuffer);
        device.transferQueue.submit(submitInfo);
        device.transferQueue.waitIdle(); // TODO: change this to wait on a fence instead (upon queue submit) so multiple memory transfers would be possible

        // free command buffer directly after use
        device.logicalDevice.freeCommandBuffers(commandPool, commandBuffer);
    }
    void transition_layout(vk::CommandBuffer commandBuffer, vk::ImageLayout from, vk::ImageLayout to) {
        vk::ImageMemoryBarrier barrier = vk::ImageMemoryBarrier()
			.setOldLayout(from)
			.setNewLayout(to)
			.setImage(image)
			.setSubresourceRange(vk::ImageSubresourceRange()
				.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setBaseArrayLayer(0).setLayerCount(1)
				.setBaseMipLevel(0).setLevelCount(1));
		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);
    }

public:
    vk::Image get_image() { return image; }
    vk::ImageView get_image_view() { return imageView; }

private:
    void create_image(vma::Allocator allocator, vk::ImageUsageFlags usage) {
        vk::ImageCreateInfo imageCreateInfo = vk::ImageCreateInfo()
			.setImageType(extent.depth > 1 ? vk::ImageType::e3D : vk::ImageType::e2D)
			.setExtent(extent)
			//
			.setMipLevels(1)
            .setArrayLayers(1)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setTiling(vk::ImageTiling::eOptimal)
			.setUsage(usage)
			.setFormat(colorFormat);

		vma::AllocationCreateInfo allocCreateInfo = vma::AllocationCreateInfo()
			.setUsage(vma::MemoryUsage::eAutoPreferDevice)
			.setFlags(vma::AllocationCreateFlagBits::eDedicatedMemory);
            
		vk::Result result = allocator.createImage(&imageCreateInfo, &allocCreateInfo, &image, &alloc, nullptr);
		if (result != vk::Result::eSuccess) VMI_ERR("Failed to create image");
    }
    void create_image_view(DeviceWrapper& device) {
        vk::ImageSubresourceRange subresourceRange = vk::ImageSubresourceRange()
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseMipLevel(0).setLevelCount(1)
			.setBaseArrayLayer(0).setLayerCount(1);

		vk::ImageViewCreateInfo imageViewInfo = vk::ImageViewCreateInfo()
			.setViewType(extent.depth > 1 ? vk::ImageViewType::e3D : vk::ImageViewType::e2D)
			.setSubresourceRange(subresourceRange)
			.setFormat(colorFormat)
			.setImage(image);
            
		// colors view
		imageView = device.logicalDevice.createImageView(imageViewInfo);
    }

public:
    const vk::Format colorFormat;

private:
    vk::Extent3D extent;
    vk::Image image;
    vk::ImageView imageView;
    vma::Allocation alloc;
};