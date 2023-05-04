#include "renderer/pipelines/disparity_compute.hpp"
#include "renderer/image_wrapper.hpp"
#include "device/device_wrapper.hpp"
#include "shaders/shaders.hpp"

void DisparityCompute::init(DeviceWrapper& device, vk::DescriptorPool descPool, ImageWrapper& inputImage, ImageWrapper& outputImage) {
    cs = ShaderManager::create_shader_module(device, disparity_cs, sizeof(disparity_cs));
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

void DisparityCompute::destroy(DeviceWrapper& device) {
    device.logicalDevice.destroyShaderModule(cs);
    
    // Stages
    device.logicalDevice.destroyPipelineLayout(pipelineLayout);
    device.logicalDevice.destroyPipeline(computePipeline);

    // descriptors
    device.logicalDevice.destroyDescriptorSetLayout(descSetLayout);
}