#include <iostream>
#include <stdexcept>
#include <array>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "core/Window.hpp"
#include "core/Logger.hpp"
#include "vulkan/VkContext.hpp"
#include "vulkan/Swapchain.hpp"
#include "vulkan/CommandPool.hpp"
#include "vulkan/RenderPass.hpp"
#include "renderer/Renderer.hpp"
#include "scene/SceneLoader.hpp"

constexpr int  WIDTH     = 1280;
constexpr int  HEIGHT    = 720;
constexpr char TITLE[]   = "VulkanEngine – PBR";

struct FrameData {
    VkFence         inFlight = VK_NULL_HANDLE;
    VkCommandBuffer cmd      = VK_NULL_HANDLE;
};

class Engine {
public:
    void run() { init(); loop(); cleanup(); }

private:
    Window      m_window { WIDTH, HEIGHT, TITLE };
    VkContext   m_ctx;
    Swapchain   m_swapchain;
    CommandPool m_cmdPool;
    RenderPass  m_renderPass;
    Renderer    m_renderer;

    std::array<FrameData,   MAX_FRAMES> m_frames;
    std::array<VkSemaphore, MAX_FRAMES> m_imageAvailableSems;
    std::array<VkSemaphore, 3>          m_renderFinishedSems; // 1 pro Swapchain-Image

    uint32_t m_currentFrame       = 0;
    bool     m_framebufferResized = false;

    // ── Init ──────────────────────────────────────────────────────────────────
    void init() {
        m_window.setResizeCallback([this](int, int) {
            m_framebufferResized = true;
        });

        m_ctx.init(m_window.getHandle());

        m_swapchain.init(m_ctx, m_window.getWidth(), m_window.getHeight());

        m_cmdPool.init(m_ctx);

        m_renderPass.init(m_ctx.getDevice(),
                          m_swapchain.getImageFormat(),
                          m_ctx.findDepthFormat());
        m_renderPass.createFramebuffers(m_ctx.getDevice(),
                                        m_swapchain.getImageViews(),
                                        m_swapchain.getDepthImageView(),
                                        m_swapchain.getExtent());

        m_renderer.init(m_ctx, m_swapchain, m_renderPass, m_cmdPool);

        SceneLoader::load("assets/scene.json", m_renderer, m_ctx, m_cmdPool);

        createSyncObjects();
        allocateCommandBuffers();

        LOG_INFO("Engine ready");
    }

    // ── Main Loop ─────────────────────────────────────────────────────────────
    void loop() {
        while (!m_window.shouldClose()) {
            m_window.pollEvents();
            drawFrame();
        }
        vkDeviceWaitIdle(m_ctx.getDevice());
    }

    // ── Draw Frame ────────────────────────────────────────────────────────────
    void drawFrame() {
        FrameData& frame = m_frames[m_currentFrame];

        // Warten bis dieser Frame-Slot frei ist
        vkWaitForFences(m_ctx.getDevice(), 1, &frame.inFlight, VK_TRUE, UINT64_MAX);

        // Nächstes Swapchain-Image holen
        uint32_t imageIndex;
        VkResult result = m_swapchain.acquireNextImage(
            m_ctx.getDevice(), m_imageAvailableSems[m_currentFrame], imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapchain();
            return;
        }
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            throw std::runtime_error("Failed to acquire swapchain image");

        vkResetFences(m_ctx.getDevice(), 1, &frame.inFlight);
        vkResetCommandBuffer(frame.cmd, 0);
        recordCommandBuffer(frame.cmd, imageIndex);

        // Submit — imageAvailable per Frame, renderFinished per Swapchain-Image
        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo si{};
        si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.waitSemaphoreCount   = 1;
        si.pWaitSemaphores      = &m_imageAvailableSems[m_currentFrame];
        si.pWaitDstStageMask    = &waitStage;
        si.commandBufferCount   = 1;
        si.pCommandBuffers      = &frame.cmd;
        si.signalSemaphoreCount = 1;
        si.pSignalSemaphores    = &m_renderFinishedSems[imageIndex];

        if (vkQueueSubmit(m_ctx.getGraphicsQueue(), 1, &si, frame.inFlight) != VK_SUCCESS)
            throw std::runtime_error("Failed to submit draw command buffer");

        // Present
        VkPresentInfoKHR pi{};
        pi.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        pi.waitSemaphoreCount = 1;
        pi.pWaitSemaphores    = &m_renderFinishedSems[imageIndex];
        VkSwapchainKHR sc     = m_swapchain.getSwapchain();
        pi.swapchainCount     = 1;
        pi.pSwapchains        = &sc;
        pi.pImageIndices      = &imageIndex;

        result = vkQueuePresentKHR(m_ctx.getPresentQueue(), &pi);

        if (result == VK_ERROR_OUT_OF_DATE_KHR ||
            result == VK_SUBOPTIMAL_KHR        ||
            m_framebufferResized)
        {
            m_framebufferResized = false;
            recreateSwapchain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to present swapchain image");
        }

        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES;
    }

    // ── Record Command Buffer ─────────────────────────────────────────────────
    void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex) {
        VkCommandBufferBeginInfo bi{};
        bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        if (vkBeginCommandBuffer(cmd, &bi) != VK_SUCCESS)
            throw std::runtime_error("Failed to begin command buffer");

        std::array<VkClearValue, 2> clear{};
        clear[0].color        = {{ 0.01f, 0.01f, 0.01f, 1.f }};
        clear[1].depthStencil = { 1.f, 0 };

        VkRenderPassBeginInfo rpi{};
        rpi.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpi.renderPass        = m_renderPass.getPass();
        rpi.framebuffer       = m_renderPass.getFramebuffer(imageIndex);
        rpi.renderArea.extent = m_swapchain.getExtent();
        rpi.clearValueCount   = static_cast<uint32_t>(clear.size());
        rpi.pClearValues      = clear.data();

        vkCmdBeginRenderPass(cmd, &rpi, VK_SUBPASS_CONTENTS_INLINE);

        m_renderer.draw(cmd, m_currentFrame, m_swapchain, m_renderPass);

        vkCmdEndRenderPass(cmd);

        if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
            throw std::runtime_error("Failed to record command buffer");
    }

    // ── Recreate Swapchain (Resize) ───────────────────────────────────────────
    void recreateSwapchain() {
        // Bei Minimierung blockieren
        int w = 0, h = 0;
        while (w == 0 || h == 0) {
            glfwGetFramebufferSize(m_window.getHandle(), &w, &h);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(m_ctx.getDevice());

        m_renderPass.destroyFramebuffers(m_ctx.getDevice());
        m_swapchain.recreate(m_ctx, w, h);
        m_renderPass.createFramebuffers(m_ctx.getDevice(),
                                        m_swapchain.getImageViews(),
                                        m_swapchain.getDepthImageView(),
                                        m_swapchain.getExtent());

        LOG_INFO("Swapchain recreated: " << w << "x" << h);
    }

    // ── Sync Objects ──────────────────────────────────────────────────────────
    void createSyncObjects() {
        VkSemaphoreCreateInfo si{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VkFenceCreateInfo     fi{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (auto& s : m_imageAvailableSems)
            if (vkCreateSemaphore(m_ctx.getDevice(), &si, nullptr, &s) != VK_SUCCESS)
                throw std::runtime_error("Failed to create imageAvailable semaphore");

        for (auto& s : m_renderFinishedSems)
            if (vkCreateSemaphore(m_ctx.getDevice(), &si, nullptr, &s) != VK_SUCCESS)
                throw std::runtime_error("Failed to create renderFinished semaphore");

        for (auto& f : m_frames)
            if (vkCreateFence(m_ctx.getDevice(), &fi, nullptr, &f.inFlight) != VK_SUCCESS)
                throw std::runtime_error("Failed to create fence");

        LOG_INFO("Sync objects created");
    }

    void allocateCommandBuffers() {
        auto bufs = m_cmdPool.allocate(m_ctx.getDevice(), MAX_FRAMES);
        for (int i = 0; i < MAX_FRAMES; ++i)
            m_frames[i].cmd = bufs[i];
    }

    // ── Cleanup ───────────────────────────────────────────────────────────────
    void cleanup() {
        m_renderer.destroy(m_ctx);

        for (auto s : m_imageAvailableSems)
            vkDestroySemaphore(m_ctx.getDevice(), s, nullptr);
        for (auto s : m_renderFinishedSems)
            vkDestroySemaphore(m_ctx.getDevice(), s, nullptr);
        for (auto& f : m_frames)
            vkDestroyFence(m_ctx.getDevice(), f.inFlight, nullptr);

        m_renderPass.destroy(m_ctx.getDevice());
        m_cmdPool.destroy(m_ctx.getDevice());
        m_swapchain.destroy(m_ctx.getDevice());
        m_ctx.destroy();

        LOG_INFO("Engine shut down cleanly");
    }
};

// ── Entry Point ───────────────────────────────────────────────────────────────
int main() {
    try {
        Engine{}.run();
    } catch (const std::exception& e) {
        LOG_ERROR(e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
