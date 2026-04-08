#pragma once
#include "VkContext.hpp"

class Image {
public:
    // Erstellt VkImage + allokiert Memory
    static void createImage(
        const VkContext&       ctx,
        uint32_t               width,
        uint32_t               height,
        VkFormat               format,
        VkImageTiling          tiling,
        VkImageUsageFlags      usage,
        VkMemoryPropertyFlags  properties,
        VkImage&               outImage,
        VkDeviceMemory&        outMemory
    );

    // Erstellt VkImageView
    static VkImageView createImageView(
        VkDevice        device,
        VkImage         image,
        VkFormat        format,
        VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
        uint32_t        mipLevels = 1
    );

    // Layout transition via command buffer
    static void transitionImageLayout(
        VkDevice      device,
        VkCommandPool pool,
        VkQueue       queue,
        VkImage       image,
        VkFormat      format,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        uint32_t      mipLevels = 1
    );

    // Buffer → Image copy
    static void copyBufferToImage(
        VkDevice      device,
        VkCommandPool pool,
        VkQueue       queue,
        VkBuffer      buffer,
        VkImage       image,
        uint32_t      width,
        uint32_t      height
    );

    // Mipmaps generieren
    static void generateMipmaps(
        const VkContext& ctx,
        VkCommandPool    pool,
        VkImage          image,
        VkFormat         format,
        int32_t          width,
        int32_t          height,
        uint32_t         mipLevels
    );

    // Lädt Textur von Disk, gibt mipLevels zurück
    static uint32_t loadTexture(
        const VkContext& ctx,
        VkCommandPool    pool,
        const char*      path,
        VkImage&         outImage,
        VkDeviceMemory&  outMemory,
        VkImageView&     outView,
        VkSampler&       outSampler
    );

    static VkSampler createSampler(const VkContext& ctx, uint32_t mipLevels);
};
