#pragma once

struct ShaderData { const unsigned char* pData; size_t size; };
struct ShaderPack { ShaderData vs, ps; };

struct ShaderManager
{
    static vk::ShaderModule create_shader_module(DeviceWrapper& device, const unsigned char* data, size_t size) {
        vk::ShaderModuleCreateInfo shaderInfo = vk::ShaderModuleCreateInfo()
            .setCodeSize(size)
            .setPCode(reinterpret_cast<const uint32_t*>(data)); // data alignment?
        return device.logicalDevice.createShaderModule(shaderInfo);
    }
    static vk::ShaderModule create_shader_module(DeviceWrapper& device, ShaderData shader) {
        vk::ShaderModuleCreateInfo shaderInfo = vk::ShaderModuleCreateInfo()
            .setCodeSize(shader.size)
            .setPCode(reinterpret_cast<const uint32_t*>(shader.pData)); // data alignment?
        return device.logicalDevice.createShaderModule(shaderInfo);
    }
};
