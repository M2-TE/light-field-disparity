#include "renderer/pipelines/swapchain_write.hpp"
#include "shaders/shaders.hpp"

void SwapchainWrite::init(DeviceWrapper& device, SwapchainWrapper& swapchain, vk::DescriptorPool descPool, ImageWrapper& inputImage) {
    create_shader_modules(device);
    create_render_pass(device, swapchain, inputImage);
    create_framebuffer(device, swapchain, inputImage);

    create_desc_set_layout(device);
    create_desc_set(device, descPool, inputImage);

    create_pipeline_layout(device);
    create_pipeline(device, swapchain);

    fullscreenRect = vk::Rect2D({ 0, 0 }, swapchain.get_extent());
}

void SwapchainWrite::destroy(DeviceWrapper& device) {
    // Shaders
    device.logicalDevice.destroyShaderModule(vs);
    device.logicalDevice.destroyShaderModule(ps);

    // Render Pass
    device.logicalDevice.destroyRenderPass(renderPass);
    for (size_t i = 0; i < framebuffers.size(); i++) {
        device.logicalDevice.destroyFramebuffer(framebuffers[i]);
    }

    // Stages
    device.logicalDevice.destroyPipelineLayout(pipelineLayout);
    device.logicalDevice.destroyPipeline(graphicsPipeline);

    // descriptors
    device.logicalDevice.destroyDescriptorSetLayout(descSetLayout);
}

void SwapchainWrite::create_shader_modules(DeviceWrapper& device) {
    vs = ShaderManager::create_shader_module(device, swapchain_write_vs, sizeof(swapchain_write_vs));
    ps = ShaderManager::create_shader_module(device, swapchain_write_ps, sizeof(swapchain_write_ps));
}