#include "Swapchain.hpp"
#include "Image.hpp"
#include "core/Logger.hpp"
#include <stdexcept>
#include <algorithm>

void Swapchain::init(VkContext& ctx, int width, int height) {
    createSwapchain(ctx, width, height);
    createImageViews(ctx.getDevice());
    createDepthResources(ctx);
    LOG_INFO("Swapchain created: " << m_extent.width << "x" << m_extent.height
             << "  images=" << m_images.size());
}

void Swapchain::recreate(VkContext& ctx, int width, int height) {
    destroy(ctx.getDevice());
    init(ctx, width, height);
}

void Swapchain::destroy(VkDevice device) {
    vkDestroyImageView(device, m_depthImageView, nullptr);
    vkDestroyImage(device, m_depthImage, nullptr);
    // Note: depth memory freed externally via Image helper

    for (auto iv : m_imageViews)
        vkDestroyImageView(device, iv, nullptr);
    m_imageViews.clear();

    vkDestroySwapchainKHR(device, m_swapchain, nullptr);
    m_swapchain = VK_NULL_HANDLE;
}

// ── Create ───────────────────────────────────────────────────────────────────
void Swapchain::createSwapchain(VkContext& ctx, int width, int height) {
    auto support = ctx.querySwapchainSupport();
    auto fmt     = chooseSurfaceFormat(support.formats);
    auto pm      = choosePresentMode(support.presentModes);
    auto ext     = chooseExtent(support.capabilities, width, height);

    uint32_t imageCount = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0)
        imageCount = std::min(imageCount, support.capabilities.maxImageCount);

    VkSwapchainCreateInfoKHR info{};
    info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface          = ctx.getSurface();
    info.minImageCount    = imageCount;
    info.imageFormat      = fmt.format;
    info.imageColorSpace  = fmt.colorSpace;
    info.imageExtent      = ext;
    info.imageArrayLayers = 1;
    info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    auto qf = ctx.getQueueFamilies();
    uint32_t qfIndices[] = { qf.graphicsFamily.value(), qf.presentFamily.value() };
    if (qf.graphicsFamily != qf.presentFamily) {
        info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = 2;
        info.pQueueFamilyIndices   = qfIndices;
    } else {
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    info.preTransform   = support.capabilities.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode    = pm;
    info.clipped        = VK_TRUE;
    info.oldSwapchain   = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(ctx.getDevice(), &info, nullptr, &m_swapchain) != VK_SUCCESS)
        throw std::runtime_error("Failed to create swapchain");

    uint32_t cnt = 0;
    vkGetSwapchainImagesKHR(ctx.getDevice(), m_swapchain, &cnt, nullptr);
    m_images.resize(cnt);
    vkGetSwapchainImagesKHR(ctx.getDevice(), m_swapchain, &cnt, m_images.data());

    m_imageFormat = fmt.format;
    m_extent      = ext;
}

void Swapchain::createImageViews(VkDevice device) {
    m_imageViews.resize(m_images.size());
    for (size_t i = 0; i < m_images.size(); ++i) {
        VkImageViewCreateInfo info{};
        info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.image                           = m_images[i];
        info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        info.format                          = m_imageFormat;
        info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        info.subresourceRange.levelCount     = 1;
        info.subresourceRange.layerCount     = 1;
        if (vkCreateImageView(device, &info, nullptr, &m_imageViews[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create image view");
    }
}

void Swapchain::createDepthResources(VkContext& ctx) {
    VkFormat depthFmt = ctx.findDepthFormat();
    Image::createImage(
        ctx,
        m_extent.width, m_extent.height,
        depthFmt,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_depthImage, m_depthImageMemory
    );
    m_depthImageView = Image::createImageView(ctx.getDevice(), m_depthImage, depthFmt,
                                              VK_IMAGE_ASPECT_DEPTH_BIT);
}

// ── Acquire ──────────────────────────────────────────────────────────────────
VkResult Swapchain::acquireNextImage(VkDevice device, VkSemaphore semaphore, uint32_t& imageIndex) {
    return vkAcquireNextImageKHR(device, m_swapchain, UINT64_MAX, semaphore, VK_NULL_HANDLE, &imageIndex);
}

// ── Choose helpers ───────────────────────────────────────────────────────────
VkSurfaceFormatKHR Swapchain::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
    for (auto& f : formats)
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return f;
    return formats[0];
}

VkPresentModeKHR Swapchain::choosePresentMode(const std::vector<VkPresentModeKHR>& modes) {
    for (auto m : modes)
        if (m == VK_PRESENT_MODE_MAILBOX_KHR) return m;   // triple buffering
    return VK_PRESENT_MODE_FIFO_KHR;                       // vsync fallback
}

VkExtent2D Swapchain::chooseExtent(const VkSurfaceCapabilitiesKHR& caps, int w, int h) {
    if (caps.currentExtent.width != UINT32_MAX) return caps.currentExtent;
    return {
        std::clamp((uint32_t)w, caps.minImageExtent.width,  caps.maxImageExtent.width),
        std::clamp((uint32_t)h, caps.minImageExtent.height, caps.maxImageExtent.height)
    };
}
