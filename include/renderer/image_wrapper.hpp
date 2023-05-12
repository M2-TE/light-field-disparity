#pragma once

#include "vma/include/vk_mem_alloc.hpp"
#include "stb/stb_image.h"

class ImageWrapper 
{
public:
    // ImageWrapper(const char* file_or_foldername) // TODO
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
   
   
    void load_buffer(DeviceWrapper& device, vma::Allocator& allocator, vk::CommandPool& commandPool, vk::Buffer buffer) {
        vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo()
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandPool(commandPool)
            .setCommandBufferCount(1);

        vk::CommandBuffer commandBuffer;
        vk::Result res = device.logicalDevice.allocateCommandBuffers(&allocInfo, &commandBuffer);
        if (res != vk::Result::eSuccess) VMI_ERR("Failed to allocate command buffer");

        // begin recording to temporary command buffer
        vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo()
            .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        commandBuffer.begin(beginInfo);

        // TODO: describe subresource here, need to select each image in lightfield array individually
        vk::BufferImageCopy region = vk::BufferImageCopy()
            // buffer
            .setBufferRowLength(extent.width)
            .setBufferImageHeight(extent.height)
            .setBufferOffset(0)
            // img
            .setImageExtent(extent)
            .setImageOffset(0)
            .setImageSubresource(vk::ImageSubresourceLayers()
                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                .setBaseArrayLayer(0).setLayerCount(1)
                .setMipLevel(0));
        
        transition_layout(commandBuffer, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
        commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
        commandBuffer.end();

        vk::SubmitInfo submitInfo = vk::SubmitInfo()
            .setCommandBufferCount(1)
            .setPCommandBuffers(&commandBuffer);
        device.transferQueue.submit(submitInfo);
        device.transferQueue.waitIdle(); // TODO: change this to wait on a fence instead (upon queue submit) so multiple memory transfers would be possible

        // free command buffer directly after use
        device.logicalDevice.freeCommandBuffers(commandPool, commandBuffer);
    }
    void load2D(DeviceWrapper& device, vma::Allocator& allocator, vk::CommandPool& commandPool, const char* filename) {
        if (!std::filesystem::exists(filename)) {
            VMI_ERR("Could not find specified image: " << filename);
        }

		int width, height, srcChannels;
		stbi_uc* pImg = stbi_load(filename, &width, &height, &srcChannels, STBI_rgb_alpha);
		vk::DeviceSize fileSize = width * height * STBI_rgb_alpha;
        
        // staging buffer
		vk::BufferCreateInfo bufferInfo = vk::BufferCreateInfo()
			.setSize(fileSize)
			.setUsage(vk::BufferUsageFlagBits::eTransferSrc);
		vma::AllocationCreateInfo allocCreateInfo = vma::AllocationCreateInfo()
			.setUsage(vma::MemoryUsage::eAuto)
			.setFlags(vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped);
		vma::AllocationInfo allocInfo;
		auto stagingBuffer = allocator.createBuffer(bufferInfo, allocCreateInfo, allocInfo);

		// copy into staging buffer
		memcpy(allocInfo.pMappedData, pImg, fileSize);

        load_buffer(device, allocator, commandPool, stagingBuffer.first);

		// clean up
		allocator.destroyBuffer(stagingBuffer.first, stagingBuffer.second);
		stbi_image_free(pImg);
    }
    void load3D(DeviceWrapper& device, vma::Allocator& allocator, vk::CommandPool& commandPool, 
            const char* foldername, const char* commonFilename, std::vector<int> imageIndices) {

        // iterate over all files/folders
        const std::filesystem::path directory{foldername};
        if (!std::filesystem::exists(directory)) {
            VMI_ERR("Could not find specified directory: " << foldername);
        }

        uint32_t nFiles = 0;
        // find number of directory entires
        for (auto const& dirEntry : std::filesystem::directory_iterator{directory}) { 
            nFiles++; 
        }
        // reserve space in array for said entries
        std::vector<std::string> files;
        files.reserve(nFiles);
        // iterate again
        for (auto const& dirEntry : std::filesystem::directory_iterator{directory}) {
            std::string file = dirEntry.path().generic_string();
            // only add file to array if it contains given substring
            if (file.find(commonFilename) != std::string::npos) {
                files.push_back(file);
            }
        }
        // resize array to proper size and sort entries
        files.shrink_to_fit();
        std::sort(files.begin(), files.end());
        // only load the requested images
        for (int i = 0; i < imageIndices.size(); i++) {
            files[i] = files[imageIndices[i]];
        }
        files.resize(imageIndices.size());

        // total/slice buffer sizes
        vk::DeviceSize fileSize = extent.width * extent.height * STBI_rgb_alpha;
        vk::DeviceSize totalSize = extent.width * extent.height * STBI_rgb_alpha * extent.depth;

        // staging buffer
		vk::BufferCreateInfo bufferInfo = vk::BufferCreateInfo()
			.setSize(totalSize)
			.setUsage(vk::BufferUsageFlagBits::eTransferSrc);
		vma::AllocationCreateInfo allocCreateInfo = vma::AllocationCreateInfo()
			.setUsage(vma::MemoryUsage::eAuto)
			.setFlags(vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped);
		vma::AllocationInfo allocInfo;
		auto stagingBuffer = allocator.createBuffer(bufferInfo, allocCreateInfo, allocInfo);
        
		// copy into staging buffer
        int srcWidth, srcHeight, srcChannels;
        char* pBuffer = reinterpret_cast<char*>(allocInfo.pMappedData);
        for (int i = 0; i < files.size(); i++) {
            stbi_uc* pImg = stbi_load(files[i].c_str(), &srcWidth, &srcHeight, &srcChannels, STBI_rgb_alpha);
            // copy into temp buffer with fileSize as stride
            memcpy(pBuffer + i * fileSize, pImg, fileSize);
		    stbi_image_free(pImg);
        }

        load_buffer(device, allocator, commandPool, stagingBuffer.first);

		// clean up
		allocator.destroyBuffer(stagingBuffer.first, stagingBuffer.second);
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