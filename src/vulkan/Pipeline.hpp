#pragma once
#include "VkContext.hpp"
#include <string>
#include <vector>

class Pipeline {
public:
    struct Config {
        VkRenderPass             renderPass    = VK_NULL_HANDLE;
        VkDescriptorSetLayout    descriptorLayout = VK_NULL_HANDLE;
        VkExtent2D               extent        = {};
        VkPrimitiveTopology      topology      = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPolygonMode            polygonMode   = VK_POLYGON_MODE_FILL;
        VkCullModeFlags          cullMode      = VK_CULL_MODE_BACK_BIT;
        bool                     depthTest     = true;
        bool                     depthWrite    = true;
    };

    void init(VkDevice device, const std::string& vertPath,
              const std::string& fragPath, const Config& cfg);
    void destroy(VkDevice device);

    VkPipeline       getPipeline() const { return m_pipeline; }
    VkPipelineLayout getLayout()   const { return m_layout; }

private:
    static std::vector<char>    readFile(const std::string& path);
    static VkShaderModule       createShaderModule(VkDevice device,
                                                   const std::vector<char>& code);

    VkPipeline       m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_layout   = VK_NULL_HANDLE;
};
