#pragma once
#include "VkContext.hpp"
#include <vector>

class RenderPass {
public:
    // Erstellt einen RenderPass mit Color + Depth Attachment
    void init(VkDevice device, VkFormat colorFormat, VkFormat depthFormat);
    void destroy(VkDevice device);

    // Erstellt Framebuffers für alle Swapchain-Images
    void createFramebuffers(VkDevice device,
                            const std::vector<VkImageView>& colorViews,
                            VkImageView depthView,
                            VkExtent2D extent);
    void destroyFramebuffers(VkDevice device);

    VkRenderPass              getPass()              const { return m_renderPass; }
    VkFramebuffer             getFramebuffer(uint32_t i) const { return m_framebuffers[i]; }

private:
    VkRenderPass                  m_renderPass   = VK_NULL_HANDLE;
    std::vector<VkFramebuffer>    m_framebuffers;
};
