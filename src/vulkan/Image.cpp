#include "Image.hpp"
#include "Buffer.hpp"
#include "core/Logger.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <stdexcept>
#include <cmath>

// ── Inline helper: single-use command buffer ─────────────────────────────────
static VkCommandBuffer beginSingleTime(VkDevice device, VkCommandPool pool) {
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
    return cb;
}

static void endSingleTime(VkDevice device, VkCommandPool pool, VkQueue queue, VkCommandBuffer cb) {
    vkEndCommandBuffer(cb);
    VkSubmitInfo si{};
    si.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers    = &cb;
    vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(device, pool, 1, &cb);
}

// ── createImage ──────────────────────────────────────────────────────────────
void Image::createImage(const VkContext& ctx,
                        uint32_t w, uint32_t h,
                        VkFormat format, VkImageTiling tiling,
                        VkImageUsageFlags usage, VkMemoryPropertyFlags props,
                        VkImage& outImage, VkDeviceMemory& outMemory)
{
    VkImageCreateInfo info{};
    info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType     = VK_IMAGE_TYPE_2D;
    info.extent        = {w, h, 1};
    info.mipLevels     = 1;
    info.arrayLayers   = 1;
    info.format        = format;
    info.tiling        = tiling;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.usage         = usage;
    info.samples       = VK_SAMPLE_COUNT_1_BIT;
    info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(ctx.getDevice(), &info, nullptr, &outImage) != VK_SUCCESS)
        throw std::runtime_error("Failed to create image");

    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(ctx.getDevice(), outImage, &req);

    VkMemoryAllocateInfo ai{};
    ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize  = req.size;
    ai.memoryTypeIndex = ctx.findMemoryType(req.memoryTypeBits, props);

    if (vkAllocateMemory(ctx.getDevice(), &ai, nullptr, &outMemory) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate image memory");

    vkBindImageMemory(ctx.getDevice(), outImage, outMemory, 0);
}

// ── createImageView ──────────────────────────────────────────────────────────
VkImageView Image::createImageView(VkDevice device, VkImage image,
                                   VkFormat format, VkImageAspectFlags aspect,
                                   uint32_t mipLevels)
{
    VkImageViewCreateInfo info{};
    info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image                           = image;
    info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    info.format                          = format;
    info.subresourceRange.aspectMask     = aspect;
    info.subresourceRange.levelCount     = mipLevels;
    info.subresourceRange.layerCount     = 1;

    VkImageView view;
    if (vkCreateImageView(device, &info, nullptr, &view) != VK_SUCCESS)
        throw std::runtime_error("Failed to create image view");
    return view;
}

// ── transitionImageLayout ────────────────────────────────────────────────────
void Image::transitionImageLayout(VkDevice device, VkCommandPool pool, VkQueue queue,
                                  VkImage image, VkFormat /*format*/,
                                  VkImageLayout oldLayout, VkImageLayout newLayout,
                                  uint32_t mipLevels)
{
    auto cb = beginSingleTime(device, pool);

    VkImageMemoryBarrier barrier{};
    barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout           = oldLayout;
    barrier.newLayout           = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image               = image;
    barrier.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, 1 };

    VkPipelineStageFlags srcStage, dstStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else throw std::runtime_error("Unsupported layout transition");

    vkCmdPipelineBarrier(cb, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    endSingleTime(device, pool, queue, cb);
}

// ── copyBufferToImage ────────────────────────────────────────────────────────
void Image::copyBufferToImage(VkDevice device, VkCommandPool pool, VkQueue queue,
                              VkBuffer buffer, VkImage image, uint32_t w, uint32_t h)
{
    auto cb = beginSingleTime(device, pool);

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent                 = {w, h, 1};

    vkCmdCopyBufferToImage(cb, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    endSingleTime(device, pool, queue, cb);
}

// ── generateMipmaps ──────────────────────────────────────────────────────────
void Image::generateMipmaps(const VkContext& ctx, VkCommandPool pool,
                             VkImage image, VkFormat /*format*/,
                             int32_t w, int32_t h, uint32_t mipLevels)
{
    auto cb = beginSingleTime(ctx.getDevice(), pool);

    VkImageMemoryBarrier barrier{};
    barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image               = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    int32_t mipW = w, mipH = h;
    for (uint32_t i = 1; i < mipLevels; ++i) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(cb,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[1]            = {mipW, mipH, 1};
        blit.srcSubresource           = { VK_IMAGE_ASPECT_COLOR_BIT, i-1, 0, 1 };
        blit.dstOffsets[1]            = {mipW > 1 ? mipW/2 : 1, mipH > 1 ? mipH/2 : 1, 1};
        blit.dstSubresource           = { VK_IMAGE_ASPECT_COLOR_BIT, i, 0, 1 };
        vkCmdBlitImage(cb, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cb,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        if (mipW > 1) mipW /= 2;
        if (mipH > 1) mipH /= 2;
    }
    // Letzte Mip-Ebene in SHADER_READ_ONLY
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cb,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    endSingleTime(ctx.getDevice(), pool, ctx.getGraphicsQueue(), cb);
}

// ── loadTexture ──────────────────────────────────────────────────────────────
uint32_t Image::loadTexture(const VkContext& ctx, VkCommandPool pool,
                            const char* path,
                            VkImage& outImage, VkDeviceMemory& outMemory,
                            VkImageView& outView, VkSampler& outSampler)
{
    int w, h, ch;
    stbi_uc* pixels = stbi_load(path, &w, &h, &ch, STBI_rgb_alpha);
    if (!pixels) throw std::runtime_error(std::string("Failed to load texture: ") + path);

    uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(w, h)))) + 1;
    VkDeviceSize size  = w * h * 4;

    // Staging
    VkBuffer       staging;
    VkDeviceMemory stagingMem;
    Buffer::create(ctx, size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging, stagingMem);

    void* data;
    vkMapMemory(ctx.getDevice(), stagingMem, 0, size, 0, &data);
    memcpy(data, pixels, size);
    vkUnmapMemory(ctx.getDevice(), stagingMem);
    stbi_image_free(pixels);

    // ── FIX: mipLevels an createImage übergeben ───────────────────────────────
    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent        = { (uint32_t)w, (uint32_t)h, 1 };
    imageInfo.mipLevels     = mipLevels;   // ← war vorher 1 via createImage-Helper
    imageInfo.arrayLayers   = 1;
    imageInfo.format        = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage         = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                              VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                              VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(ctx.getDevice(), &imageInfo, nullptr, &outImage) != VK_SUCCESS)
        throw std::runtime_error("Failed to create texture image");

    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(ctx.getDevice(), outImage, &req);
    VkMemoryAllocateInfo ai{};
    ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize  = req.size;
    ai.memoryTypeIndex = ctx.findMemoryType(req.memoryTypeBits,
                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(ctx.getDevice(), &ai, nullptr, &outMemory) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate texture memory");
    vkBindImageMemory(ctx.getDevice(), outImage, outMemory, 0);
    // ─────────────────────────────────────────────────────────────────────────

    transitionImageLayout(ctx.getDevice(), pool, ctx.getGraphicsQueue(),
        outImage, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);

    copyBufferToImage(ctx.getDevice(), pool, ctx.getGraphicsQueue(),
        staging, outImage, w, h);

    generateMipmaps(ctx, pool, outImage, VK_FORMAT_R8G8B8A8_SRGB, w, h, mipLevels);

    vkDestroyBuffer(ctx.getDevice(), staging, nullptr);
    vkFreeMemory(ctx.getDevice(), stagingMem, nullptr);

    outView    = createImageView(ctx.getDevice(), outImage, VK_FORMAT_R8G8B8A8_SRGB,
                                 VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
    outSampler = createSampler(ctx, mipLevels);

    LOG_INFO("Texture loaded: " << path << "  " << w << "x" << h
             << "  mips=" << mipLevels);
    return mipLevels;
}
// ── createSampler ────────────────────────────────────────────────────────────
VkSampler Image::createSampler(const VkContext& ctx, uint32_t mipLevels) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(ctx.getPhysicalDevice(), &props);

    VkSamplerCreateInfo info{};
    info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter               = VK_FILTER_LINEAR;
    info.minFilter               = VK_FILTER_LINEAR;
    info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.anisotropyEnable        = VK_TRUE;
    info.maxAnisotropy           = props.limits.maxSamplerAnisotropy;
    info.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    info.compareOp               = VK_COMPARE_OP_ALWAYS;
    info.minLod                  = 0.0f;
    info.maxLod                  = static_cast<float>(mipLevels);

    VkSampler sampler;
    if (vkCreateSampler(ctx.getDevice(), &info, nullptr, &sampler) != VK_SUCCESS)
        throw std::runtime_error("Failed to create sampler");
    return sampler;
}
