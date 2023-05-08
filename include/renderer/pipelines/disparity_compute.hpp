#pragma once

#include "device/device_wrapper.hpp"
#include "renderer/image_wrapper.hpp"

class DisparityCompute 
{
public:
    void init(DeviceWrapper& device, vk::DescriptorPool descPool, ImageWrapper& inputImage, ImageWrapper& outputImage);
    void destroy(DeviceWrapper& device);
    void execute(vk::CommandBuffer commandBuffer) {

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, descSet, {});
        commandBuffer.dispatch(512 / 32, 512 / 32, 1);
    }

private:
    void create_layout_bindings(DeviceWrapper& device, vk::DescriptorPool descPool, ImageWrapper& inputImage, ImageWrapper& outputImage) {
        // set binding layouts
        std::array<vk::DescriptorSetLayoutBinding, 2> bindings;
		bindings[0] = vk::DescriptorSetLayoutBinding()
			.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eSampledImage)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		bindings[1] = vk::DescriptorSetLayoutBinding()
			.setBinding(1)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eStorageImage)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		vk::DescriptorSetLayoutCreateInfo createInfo = vk::DescriptorSetLayoutCreateInfo()
			.setBindings(bindings);
		descSetLayout = device.logicalDevice.createDescriptorSetLayout(createInfo);

        // allocate the descriptor sets using descriptor pool
        vk::DescriptorSetAllocateInfo allocInfo = vk::DescriptorSetAllocateInfo()
            .setDescriptorPool(descPool)
            .setDescriptorSetCount(1).setPSetLayouts(&descSetLayout);
        descSet = device.logicalDevice.allocateDescriptorSets(allocInfo)[0];

        // input image
        vk::DescriptorImageInfo descriptor = vk::DescriptorImageInfo()
            .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setImageView(inputImage.get_image_view())
            .setSampler(nullptr);
        vk::WriteDescriptorSet descBufferWrites = vk::WriteDescriptorSet()
            .setDstSet(descSet)
            .setDstBinding(0)
            .setDstArrayElement(0)
            .setDescriptorType(vk::DescriptorType::eSampledImage)
            .setImageInfo(descriptor);
        device.logicalDevice.updateDescriptorSets(descBufferWrites, {});

        // output image
        descriptor = vk::DescriptorImageInfo()
            .setImageLayout(vk::ImageLayout::eGeneral)
            .setImageView(outputImage.get_image_view())
            .setSampler(nullptr);
        descBufferWrites = vk::WriteDescriptorSet()
            .setDstSet(descSet)
            .setDstBinding(1)
            .setDstArrayElement(0)
            .setDescriptorType(vk::DescriptorType::eStorageImage)
            .setImageInfo(descriptor);
        device.logicalDevice.updateDescriptorSets(descBufferWrites, {});

        // create pipeline layout
        vk::PipelineLayoutCreateInfo layoutInfo = vk::PipelineLayoutCreateInfo()
            .setSetLayouts(descSetLayout);
        pipelineLayout = device.logicalDevice.createPipelineLayout(layoutInfo);
    }

private:
    vk::Pipeline computePipeline;
	vk::PipelineCache pipelineCache; // TODO
    vk::PipelineLayout pipelineLayout;

	vk::DescriptorSetLayout descSetLayout;
	vk::DescriptorSet descSet;

    vk::ShaderModule cs;
};