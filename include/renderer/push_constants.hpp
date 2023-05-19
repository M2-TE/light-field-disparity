#pragma once 

struct alignas(4) PushConstants {
    static vk::PushConstantRange get_range() {
         return vk::PushConstantRange()
            .setSize(sizeof(PushConstants))
            .setStageFlags(vk::ShaderStageFlagBits::eCompute)
            .setOffset(0);
    }

    uint32_t iPhase = 0;
    uint32_t nSteps = 0;
};