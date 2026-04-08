#pragma once
#include "VkContext.hpp"
#include <vector>

class Swapchain {
public:
    void init(VkContext& ctx, int width, int height);
    void recreate(VkContext& ctx, int width, int height);
    void destroy(VkDevice device);

    VkSwapchainKHR           getSwapchain()             const { return m_swapchain; }
    VkFormat                 getImageFormat()            const { return m_imageFormat; }
    VkExtent2D               getExtent()                 const { return m_extent; }
    uint32_t                 getImageCount()             const { return static_cast<uint32_t>(m_images.size()); }
    VkImageView              getImageView(uint32_t i)   const { return m_imageViews[i]; }
    const std::vector<VkImageView>& getImageViews()     const { return m_imageViews; }

    // Depth
    VkImage     getDepthImage()     const { return m_depthImage; }
    VkImageView getDepthImageView() const { return m_depthImageView; }

    VkResult acquireNextImage(VkDevice device, VkSemaphore semaphore, uint32_t& imageIndex);

private:
    void createSwapchain(VkContext& ctx, int width, int height);
    void createImageViews(VkDevice device);
    void createDepthResources(VkContext& ctx);

    VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR   choosePresentMode(const std::vector<VkPresentModeKHR>& modes);
    VkExtent2D         chooseExtent(const VkSurfaceCapabilitiesKHR& caps, int w, int h);

    VkSwapchainKHR           m_swapchain    = VK_NULL_HANDLE;
    VkFormat                 m_imageFormat  = VK_FORMAT_UNDEFINED;
    VkExtent2D               m_extent       = {};
    std::vector<VkImage>     m_images;
    std::vector<VkImageView> m_imageViews;

    // Depth
    VkImage        m_depthImage       = VK_NULL_HANDLE;
    VkDeviceMemory m_depthImageMemory = VK_NULL_HANDLE;
    VkImageView    m_depthImageView   = VK_NULL_HANDLE;
};
