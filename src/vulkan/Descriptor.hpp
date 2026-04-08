#pragma once
#include "VkContext.hpp"
#include <vector>

class Descriptor {
public:
    // Layout
    static VkDescriptorSetLayout createLayout(
        VkDevice device,
        const std::vector<VkDescriptorSetLayoutBinding>& bindings
    );

    // Pool
    void createPool(VkDevice device, uint32_t maxSets,
                    const std::vector<VkDescriptorPoolSize>& sizes);
    void destroyPool(VkDevice device);

    // Sets
    std::vector<VkDescriptorSet> allocateSets(
        VkDevice device,
        VkDescriptorSetLayout layout,
        uint32_t count
    );

    // Convenience: UBO-Binding schreiben
    static void writeUBO(VkDevice device, VkDescriptorSet set,
                         uint32_t binding, VkBuffer buffer, VkDeviceSize size);

    // Convenience: Combined Image Sampler schreiben
    static void writeImageSampler(VkDevice device, VkDescriptorSet set,
                                  uint32_t binding, VkImageView view, VkSampler sampler);

private:
    VkDescriptorPool m_pool = VK_NULL_HANDLE;
};
