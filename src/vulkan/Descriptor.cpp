#include "Descriptor.hpp"
#include <stdexcept>

VkDescriptorSetLayout Descriptor::createLayout(
    VkDevice device,
    const std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
    VkDescriptorSetLayoutCreateInfo info{};
    info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = static_cast<uint32_t>(bindings.size());
    info.pBindings    = bindings.data();

    VkDescriptorSetLayout layout;
    if (vkCreateDescriptorSetLayout(device, &info, nullptr, &layout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor set layout");
    return layout;
}

void Descriptor::createPool(VkDevice device, uint32_t maxSets,
                             const std::vector<VkDescriptorPoolSize>& sizes)
{
    VkDescriptorPoolCreateInfo info{};
    info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.maxSets       = maxSets;
    info.poolSizeCount = static_cast<uint32_t>(sizes.size());
    info.pPoolSizes    = sizes.data();

    if (vkCreateDescriptorPool(device, &info, nullptr, &m_pool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor pool");
}

void Descriptor::destroyPool(VkDevice device) {
    vkDestroyDescriptorPool(device, m_pool, nullptr);
}

std::vector<VkDescriptorSet> Descriptor::allocateSets(
    VkDevice device, VkDescriptorSetLayout layout, uint32_t count)
{
    std::vector<VkDescriptorSetLayout> layouts(count, layout);
    VkDescriptorSetAllocateInfo ai{};
    ai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool     = m_pool;
    ai.descriptorSetCount = count;
    ai.pSetLayouts        = layouts.data();

    std::vector<VkDescriptorSet> sets(count);
    if (vkAllocateDescriptorSets(device, &ai, sets.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate descriptor sets");
    return sets;
}

void Descriptor::writeUBO(VkDevice device, VkDescriptorSet set,
                           uint32_t binding, VkBuffer buffer, VkDeviceSize size)
{
    VkDescriptorBufferInfo bi{ buffer, 0, size };
    VkWriteDescriptorSet w{};
    w.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w.dstSet          = set;
    w.dstBinding      = binding;
    w.descriptorCount = 1;
    w.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    w.pBufferInfo     = &bi;
    vkUpdateDescriptorSets(device, 1, &w, 0, nullptr);
}

void Descriptor::writeImageSampler(VkDevice device, VkDescriptorSet set,
                                    uint32_t binding, VkImageView view, VkSampler sampler)
{
    VkDescriptorImageInfo ii{ sampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    VkWriteDescriptorSet w{};
    w.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w.dstSet          = set;
    w.dstBinding      = binding;
    w.descriptorCount = 1;
    w.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w.pImageInfo      = &ii;
    vkUpdateDescriptorSets(device, 1, &w, 0, nullptr);
}
