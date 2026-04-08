#include "CommandPool.hpp"
#include "core/Logger.hpp"
#include <stdexcept>

void CommandPool::init(const VkContext& ctx) {
    VkCommandPoolCreateInfo info{};
    info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.queueFamilyIndex = ctx.getQueueFamilies().graphicsFamily.value();
    info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(ctx.getDevice(), &info, nullptr, &m_pool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create command pool");

    LOG_INFO("Command pool created");
}

void CommandPool::destroy(VkDevice device) {
    vkDestroyCommandPool(device, m_pool, nullptr);
}

std::vector<VkCommandBuffer> CommandPool::allocate(VkDevice device, uint32_t count) {
    VkCommandBufferAllocateInfo ai{};
    ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool        = m_pool;
    ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = count;

    std::vector<VkCommandBuffer> buffers(count);
    if (vkAllocateCommandBuffers(device, &ai, buffers.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffers");
    return buffers;
}
