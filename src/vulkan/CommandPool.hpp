#pragma once
#include "VkContext.hpp"
#include <vector>

class CommandPool {
public:
    void init(const VkContext& ctx);
    void destroy(VkDevice device);

    VkCommandPool getPool() const { return m_pool; }

    // Allokiert `count` primary command buffers
    std::vector<VkCommandBuffer> allocate(VkDevice device, uint32_t count);

private:
    VkCommandPool m_pool = VK_NULL_HANDLE;
};
