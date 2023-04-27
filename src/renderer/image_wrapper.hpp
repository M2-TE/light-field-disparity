class ImageWrapper 
{
public:
    ImageWrapper(vk::Format colorFormat = vk::Format::eR8G8B8A8Srgb) : colorFormat(colorFormat) {}
    ~ImageWrapper() = default;

public:
    void init(DeviceWrapper& device, SwapchainWrapper& swapchain, vma::Allocator allocator) {
        create_image(swapchain, allocator);
        create_image_view(device);
    }
    void destroy(DeviceWrapper& device, vma::Allocator allocator) {
        allocator.destroyImage(image, alloc);
        device.logicalDevice.destroyImageView(imageView);
    }

public:
    vk::Image get_image() { return image; }
    vk::ImageView get_image_view() { return imageView; }

private:
    void create_image(SwapchainWrapper& swapchain, vma::Allocator allocator) {
        vk::ImageCreateInfo imageCreateInfo = vk::ImageCreateInfo()
			.setImageType(vk::ImageType::e2D)
			.setExtent(vk::Extent3D(swapchain.get_extent(), 1))
			//
			.setMipLevels(1)
            .setArrayLayers(1)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setTiling(vk::ImageTiling::eOptimal)
			.setUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled
                | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eInputAttachment)
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
			.setViewType(vk::ImageViewType::e2D)
			.setSubresourceRange(subresourceRange)
			.setFormat(colorFormat)
			.setImage(image);

		// colors view
		imageView = device.logicalDevice.createImageView(imageViewInfo);
    }

public:
    const vk::Format colorFormat;

private:
    vk::Image image;
    vk::ImageView imageView;
    vma::Allocation alloc;
};