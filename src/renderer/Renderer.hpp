#pragma once
#include "vulkan/VkContext.hpp"
#include "vulkan/Swapchain.hpp"
#include "vulkan/RenderPass.hpp"
#include "vulkan/CommandPool.hpp"
#include "vulkan/Pipeline.hpp"
#include "vulkan/Descriptor.hpp"
#include "vulkan/Buffer.hpp"
#include "renderer/Mesh.hpp"
#include "renderer/Camera.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <array>

constexpr int MAX_FRAMES = 2;

// ── UBO Layout (Binding 0) ────────────────────────────────────────────────────
struct SceneUBO {
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec4 camPos;       // w unused
    // Point lights (max 4)
    glm::vec4 lightPos[4];  // w unused
    glm::vec4 lightColor[4];// w = intensity
    int       numLights;
    float     _pad[3];
};

class Renderer {
public:
    void init(VkContext& ctx, Swapchain& swapchain,
              RenderPass& renderPass, CommandPool& cmdPool);
    void destroy(VkContext& ctx);

    // Lädt Mesh + Textur-Set (albedo, normal, metalRough, ao)
    void loadScene(VkContext& ctx, CommandPool& cmdPool,
                   const std::string& meshPath,
                   const std::string& albedoPath,
                   const std::string& normalPath,
                   const std::string& metalRoughPath,
                   const std::string& aoPath);

    // Wird pro Frame aus main.cpp aufgerufen
    void draw(VkCommandBuffer cmd, uint32_t frameIndex,
              Swapchain& swapchain, RenderPass& renderPass);

    Camera camera;

    // Lights setzen
    void setLight(int i, glm::vec3 pos, glm::vec3 color, float intensity);
    int  numLights = 1;

private:
    void createUBOs(VkContext& ctx);
    void createDescriptors(VkContext& ctx);
    void createPipeline(VkContext& ctx, Swapchain& swapchain, RenderPass& renderPass);
    void updateUBO(uint32_t frameIndex, VkExtent2D extent);

    // Pipeline
    Pipeline            m_pipeline;
    VkDescriptorSetLayout m_descLayout = VK_NULL_HANDLE;

    // Descriptors
    Descriptor          m_descriptor;
    std::vector<VkDescriptorSet> m_descSets;

    // Per-frame UBOs
    struct FrameUBO {
        VkBuffer       buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        void*          mapped = nullptr;
    };
    std::array<FrameUBO, MAX_FRAMES> m_ubos;

    // Scene
    Mesh m_mesh;

    // PBR Textures
    struct Texture {
        VkImage        image   = VK_NULL_HANDLE;
        VkDeviceMemory memory  = VK_NULL_HANDLE;
        VkImageView    view    = VK_NULL_HANDLE;
        VkSampler      sampler = VK_NULL_HANDLE;
    };
    Texture m_albedo, m_normal, m_metalRough, m_ao;

    // Lights
    glm::vec4 m_lightPos  [4] = {};
    glm::vec4 m_lightColor[4] = {};

    bool m_sceneLoaded = false;
};
