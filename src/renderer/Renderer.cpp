#include "Renderer.hpp"
#include "vulkan/Image.hpp"
#include "core/Logger.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>
#include <cstring>

// ── Init ──────────────────────────────────────────────────────────────────────
void Renderer::init(VkContext& ctx, Swapchain& swapchain,
                    RenderPass& renderPass, CommandPool& cmdPool)
{
    // Default light
    setLight(0, {2.f, 3.f, 2.f}, {1.f, 1.f, 1.f}, 10.f);

    createUBOs(ctx);
    createDescriptors(ctx);          // Layout + Pool (ohne Texturen noch)
    createPipeline(ctx, swapchain, renderPass);

    LOG_INFO("Renderer initialized");
}

void Renderer::destroy(VkContext& ctx) {
    VkDevice dev = ctx.getDevice();

    // Mesh + Texturen
    if (m_sceneLoaded) {
        m_mesh.destroy(dev);
        for (auto* tex : {&m_albedo, &m_normal, &m_metalRough, &m_ao}) {
            vkDestroySampler (dev, tex->sampler, nullptr);
            vkDestroyImageView(dev, tex->view,   nullptr);
            vkDestroyImage   (dev, tex->image,   nullptr);
            vkFreeMemory     (dev, tex->memory,  nullptr);
        }
    }

    // UBOs
    for (auto& ubo : m_ubos) {
        vkUnmapMemory   (dev, ubo.memory);
        vkDestroyBuffer (dev, ubo.buffer, nullptr);
        vkFreeMemory    (dev, ubo.memory, nullptr);
    }

    m_descriptor.destroyPool(dev);
    vkDestroyDescriptorSetLayout(dev, m_descLayout, nullptr);
    m_pipeline.destroy(dev);
}

// ── UBOs ──────────────────────────────────────────────────────────────────────
void Renderer::createUBOs(VkContext& ctx) {
    VkDeviceSize size = sizeof(SceneUBO);
    for (auto& ubo : m_ubos) {
        Buffer::create(ctx, size,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            ubo.buffer, ubo.memory);
        vkMapMemory(ctx.getDevice(), ubo.memory, 0, size, 0, &ubo.mapped);
    }
}

// ── Descriptors ───────────────────────────────────────────────────────────────
void Renderer::createDescriptors(VkContext& ctx) {
    // Layout:
    // binding 0 = UBO          (vertex + fragment)
    // binding 1 = albedo        (fragment)
    // binding 2 = normal map    (fragment)
    // binding 3 = metalRough    (fragment)
    // binding 4 = AO            (fragment)
    std::vector<VkDescriptorSetLayoutBinding> bindings = {
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
            VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
            VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
            VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
            VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
    };
    m_descLayout = Descriptor::createLayout(ctx.getDevice(), bindings);

    std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         MAX_FRAMES},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_FRAMES * 4},
    };
    m_descriptor.createPool(ctx.getDevice(), MAX_FRAMES, poolSizes);

    m_descSets = m_descriptor.allocateSets(ctx.getDevice(), m_descLayout, MAX_FRAMES);

    // UBO bindings schreiben (Texturen kommen nach loadScene)
    for (int i = 0; i < MAX_FRAMES; ++i)
        Descriptor::writeUBO(ctx.getDevice(), m_descSets[i], 0,
                             m_ubos[i].buffer, sizeof(SceneUBO));
}

// ── Pipeline ──────────────────────────────────────────────────────────────────
void Renderer::createPipeline(VkContext& ctx, Swapchain& swapchain,
                               RenderPass& renderPass)
{
    Pipeline::Config cfg{};
    cfg.renderPass       = renderPass.getPass();
    cfg.descriptorLayout = m_descLayout;
    cfg.extent           = swapchain.getExtent();

    m_pipeline.init(ctx.getDevice(),
                    "shaders/pbr.vert.spv",
                    "shaders/pbr.frag.spv",
                    cfg);
}

// ── Load Scene ────────────────────────────────────────────────────────────────
void Renderer::loadScene(VkContext& ctx, CommandPool& cmdPool,
                         const std::string& meshPath,
                         const std::string& albedoPath,
                         const std::string& normalPath,
                         const std::string& metalRoughPath,
                         const std::string& aoPath)
{
    VkCommandPool pool = cmdPool.getPool();

    m_mesh.loadOBJ(ctx, pool, meshPath);

    auto loadTex = [&](Texture& tex, const char* path) {
        Image::loadTexture(ctx, pool, path,
                           tex.image, tex.memory, tex.view, tex.sampler);
    };
    loadTex(m_albedo,     albedoPath.c_str());
    loadTex(m_normal,     normalPath.c_str());
    loadTex(m_metalRough, metalRoughPath.c_str());
    loadTex(m_ao,         aoPath.c_str());

    // Textur-Descriptors für alle Frames schreiben
    for (int i = 0; i < MAX_FRAMES; ++i) {
        Descriptor::writeImageSampler(ctx.getDevice(), m_descSets[i],
                                      1, m_albedo.view,     m_albedo.sampler);
        Descriptor::writeImageSampler(ctx.getDevice(), m_descSets[i],
                                      2, m_normal.view,     m_normal.sampler);
        Descriptor::writeImageSampler(ctx.getDevice(), m_descSets[i],
                                      3, m_metalRough.view, m_metalRough.sampler);
        Descriptor::writeImageSampler(ctx.getDevice(), m_descSets[i],
                                      4, m_ao.view,         m_ao.sampler);
    }

    m_sceneLoaded = true;
    LOG_INFO("Scene loaded");
}

// ── Update UBO ────────────────────────────────────────────────────────────────
void Renderer::updateUBO(uint32_t frameIndex, VkExtent2D extent) {
    float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);

    SceneUBO ubo{};
    ubo.view      = camera.getView();
    ubo.proj      = camera.getProjection(aspect);
    ubo.camPos    = glm::vec4(camera.position, 1.f);
    ubo.numLights = numLights;

    for (int i = 0; i < 4; ++i) {
        ubo.lightPos[i]   = m_lightPos[i];
        ubo.lightColor[i] = m_lightColor[i];
    }

    memcpy(m_ubos[frameIndex].mapped, &ubo, sizeof(ubo));
}

// ── Draw ──────────────────────────────────────────────────────────────────────
void Renderer::draw(VkCommandBuffer cmd, uint32_t frameIndex,
                    Swapchain& swapchain, RenderPass& /*renderPass*/)
{
    VkExtent2D extent = swapchain.getExtent();
    updateUBO(frameIndex, extent);

    // Dynamischer Viewport + Scissor
    VkViewport vp{};
    vp.width    = static_cast<float>(extent.width);
    vp.height   = static_cast<float>(extent.height);
    vp.minDepth = 0.f;
    vp.maxDepth = 1.f;
    vkCmdSetViewport(cmd, 0, 1, &vp);

    VkRect2D scissor{ {0,0}, extent };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.getPipeline());

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_pipeline.getLayout(), 0, 1,
                            &m_descSets[frameIndex], 0, nullptr);

    // Push Constant: Identity Model-Matrix
    glm::mat4 model(1.f);
    vkCmdPushConstants(cmd, m_pipeline.getLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model);

    if (m_sceneLoaded) {
        m_mesh.bind(cmd);
        m_mesh.draw(cmd);
    }
}

// ── Lights ────────────────────────────────────────────────────────────────────
void Renderer::setLight(int i, glm::vec3 pos, glm::vec3 color, float intensity) {
    if (i < 0 || i >= 4) return;
    m_lightPos[i]   = glm::vec4(pos, 0.f);
    m_lightColor[i] = glm::vec4(color, intensity);
}
