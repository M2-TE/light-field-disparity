#pragma once

class DisparityCompute 
{
public:
    void init(DeviceWrapper& device) {

        cs = create_shader_module(device, disparity_cs, sizeof(disparity_cs));
        vk::PipelineShaderStageCreateInfo shaderInfo = vk::PipelineShaderStageCreateInfo()
            .setStage(vk::ShaderStageFlagBits::eVertex)
            .setModule(cs)
            .setPName("main");

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

    }

private:
    void test() {

    }

private:
    vk::Pipeline computePipeline;
	vk::PipelineCache pipelineCache; // TODO
    vk::PipelineLayout pipelineLayout;
    vk::ShaderModule cs;
};