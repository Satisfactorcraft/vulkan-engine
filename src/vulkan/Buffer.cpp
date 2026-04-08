#include "Buffer.hpp"
#include <stdexcept>
#include <cstring>

void Buffer::create(const VkContext& ctx,
                    VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags props,
                    VkBuffer& outBuffer, VkDeviceMemory& outMemory)
{
    if (size == 0)
        throw std::runtime_error("Buffer::create called with size 0 — mesh probably empty");

    VkBufferCreateInfo info{};
    info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size        = size;
    info.usage       = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(ctx.getDevice(), &info, nullptr, &outBuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to create buffer");

    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(ctx.getDevice(), outBuffer, &req);

    VkMemoryAllocateInfo ai{};
    ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize  = req.size;
    ai.memoryTypeIndex = ctx.findMemoryType(req.memoryTypeBits, props);

    if (vkAllocateMemory(ctx.getDevice(), &ai, nullptr, &outMemory) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate buffer memory");

    vkBindBufferMemory(ctx.getDevice(), outBuffer, outMemory, 0);
}

void Buffer::copy(VkDevice device, VkCommandPool pool, VkQueue queue,
                  VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
    VkCommandBufferAllocateInfo ai{};
    ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool        = pool;
    ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 1;

    VkCommandBuffer cb;
    vkAllocateCommandBuffers(device, &ai, &cb);

    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cb, &bi);

    VkBufferCopy region{};
    region.size = size;
    vkCmdCopyBuffer(cb, src, dst, 1, &region);

    vkEndCommandBuffer(cb);

    VkSubmitInfo si{};
    si.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers    = &cb;
    vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, pool, 1, &cb);
}

void Buffer::createAndUpload(const VkContext& ctx, VkCommandPool pool,
                             const void* data, VkDeviceSize size,
                             VkBufferUsageFlags usage,
                             VkBuffer& outBuffer, VkDeviceMemory& outMemory)
{
    if (size == 0)
        throw std::runtime_error("Buffer::createAndUpload called with size 0");

    // Staging Buffer — CPU sichtbar
    VkBuffer       staging;
    VkDeviceMemory stagingMem;
    create(ctx, size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging, stagingMem);

    // Daten in Staging kopieren
    void* mapped;
    vkMapMemory(ctx.getDevice(), stagingMem, 0, size, 0, &mapped);
    memcpy(mapped, data, static_cast<size_t>(size));
    vkUnmapMemory(ctx.getDevice(), stagingMem);

    // Device-local Buffer
    create(ctx, size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        outBuffer, outMemory);

    // Staging → Device
    copy(ctx.getDevice(), pool, ctx.getGraphicsQueue(), staging, outBuffer, size);

    // Staging aufräumen
    vkDestroyBuffer(ctx.getDevice(), staging, nullptr);
    vkFreeMemory   (ctx.getDevice(), stagingMem, nullptr);
}
