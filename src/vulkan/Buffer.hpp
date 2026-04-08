#pragma once
#include "VkContext.hpp"

class Buffer {
public:
    // Allokiert VkBuffer + Memory
    static void create(
        const VkContext&       ctx,
        VkDeviceSize           size,
        VkBufferUsageFlags     usage,
        VkMemoryPropertyFlags  properties,
        VkBuffer&              outBuffer,
        VkDeviceMemory&        outMemory
    );

    // Kopiert Buffer via single-use command buffer
    static void copy(
        VkDevice      device,
        VkCommandPool pool,
        VkQueue       queue,
        VkBuffer      src,
        VkBuffer      dst,
        VkDeviceSize  size
    );

    // Bequemlichkeit: erstellt device-local Buffer und lädt Daten hoch
    static void createAndUpload(
        const VkContext&       ctx,
        VkCommandPool          pool,
        const void*            data,
        VkDeviceSize           size,
        VkBufferUsageFlags     usage,        // z.B. VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
        VkBuffer&              outBuffer,
        VkDeviceMemory&        outMemory
    );
};
