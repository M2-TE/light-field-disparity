#pragma once

class DisparityCompute 
{
public:
    void init(DeviceWrapper& device, vk::DescriptorPool descPool, ImageWrapper& inputImage, ImageWrapper& outputImage) {
        cs = create_shader_module(device, disparity_cs, sizeof(disparity_cs));
        vk::PipelineShaderStageCreateInfo shaderInfo = vk::PipelineShaderStageCreateInfo()
            .setStage(vk::ShaderStageFlagBits::eCompute)
            .setModule(cs)
            .setPName("main");

        create_layout_bindings(device, descPool, inputImage, outputImage);

        vk::ComputePipelineCreateInfo pipelineInfo = vk::ComputePipelineCreateInfo()
            .setLayout(pipelineLayout)
            .setStage(shaderInfo);

        auto result = device.logicalDevice.createComputePipeline(pipelineCache, pipelineInfo);
        switch (result.result)
		{
			case vk::Result::eSuccess: break;
			case vk::Result::ePipelineCompileRequiredEXT:
				VMI_LOG("Compute pipeline creation: PipelineCompileRequiredEXT");
				break;
			default: assert(false);
		}
		computePipeline = result.value;
    }
    void destroy(DeviceWrapper& device) {
		device.logicalDevice.destroyShaderModule(cs);
        
		// Stages
		device.logicalDevice.destroyPipelineLayout(pipelineLayout);
		device.logicalDevice.destroyPipeline(computePipeline);

		// descriptors
		device.logicalDevice.destroyDescriptorSetLayout(descSetLayout);
    }
    void execute(vk::CommandBuffer commandBuffer) {

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, descSet, {});
        commandBuffer.dispatch(512, 512, 1);
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
        // set bindings

        {
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
        }

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