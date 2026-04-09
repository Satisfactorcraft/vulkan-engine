#include "Pipeline.hpp"
#include "renderer/Mesh.hpp"
#include "core/Logger.hpp"
#include <fstream>
#include <stdexcept>
#include <array>

// ── readFile ──────────────────────────────────────────────────────────────────
std::vector<char> Pipeline::readFile(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Failed to open shader: " + path);

    size_t size = file.tellg();
    std::vector<char> buf(size);
    file.seekg(0);
    file.read(buf.data(), size);
    return buf;
}

// ── createShaderModule ────────────────────────────────────────────────────────
VkShaderModule Pipeline::createShaderModule(VkDevice device,
                                             const std::vector<char>& code)
{
    if (code.empty())
        throw std::runtime_error("Shader code is empty — SPV missing or corrupt");
    if (code.size() % 4 != 0)
        throw std::runtime_error("Shader code not 4-byte aligned — corrupt SPV");

    VkShaderModuleCreateInfo info{};
    info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = code.size();
    info.pCode    = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule mod;
    if (vkCreateShaderModule(device, &info, nullptr, &mod) != VK_SUCCESS)
        throw std::runtime_error("Failed to create shader module");
    return mod;
}

// ── init ──────────────────────────────────────────────────────────────────────
void Pipeline::init(VkDevice device,
                    const std::string& vertPath,
                    const std::string& fragPath,
                    const Config& cfg)
{
    auto vertCode = readFile(vertPath);
    auto fragCode = readFile(fragPath);

    LOG_INFO("Shader sizes — vert: " << vertCode.size()
             << "  frag: " << fragCode.size());

    auto vertMod = createShaderModule(device, vertCode);
    auto fragMod = createShaderModule(device, fragCode);

    // ── Shader Stages ─────────────────────────────────────────────────────────
    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertMod;
    stages[0].pName  = "main";
    stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragMod;
    stages[1].pName  = "main";

    // ── Vertex Input ──────────────────────────────────────────────────────────
    VkVertexInputBindingDescription binding{};
    binding.binding   = 0;
    binding.stride    = sizeof(Vertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 4> attrs{};
    attrs[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)     };
    attrs[1] = { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)  };
    attrs[2] = { 2, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex, uv)      };
    attrs[3] = { 3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent) };

    LOG_INFO("Vertex stride: " << sizeof(Vertex)
             << "  offsets: pos=" << offsetof(Vertex, pos)
             << " normal="        << offsetof(Vertex, normal)
             << " uv="            << offsetof(Vertex, uv)
             << " tangent="       << offsetof(Vertex, tangent));

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount   = 1;
    vertexInput.pVertexBindingDescriptions      = &binding;
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrs.size());
    vertexInput.pVertexAttributeDescriptions    = attrs.data();

    // ── Input Assembly ────────────────────────────────────────────────────────
    VkPipelineInputAssemblyStateCreateInfo ia{};
    ia.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = cfg.topology;

    // ── Viewport (statisch) ───────────────────────────────────────────────────
    VkViewport vp{};
    vp.x        = 0.0f;
    vp.y        = 0.0f;
    vp.width    = static_cast<float>(cfg.extent.width);
    vp.height   = static_cast<float>(cfg.extent.height);
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = cfg.extent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports    = &vp;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &scissor;

    // ── Rasterizer ────────────────────────────────────────────────────────────
    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = cfg.polygonMode;
    raster.cullMode    = VK_CULL_MODE_NONE;
    raster.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth   = 1.0f;

    // ── Multisampling ─────────────────────────────────────────────────────────
    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // ── Depth / Stencil ───────────────────────────────────────────────────────
    VkPipelineDepthStencilStateCreateInfo ds{};
    ds.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable  = cfg.depthTest  ? VK_TRUE : VK_FALSE;
    ds.depthWriteEnable = cfg.depthWrite ? VK_TRUE : VK_FALSE;
    ds.depthCompareOp   = VK_COMPARE_OP_LESS;

    // ── Color Blend ───────────────────────────────────────────────────────────
    VkPipelineColorBlendAttachmentState blendAttach{};
    blendAttach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                 VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttach.blendEnable    = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments    = &blendAttach;

    // ── Pipeline Layout ───────────────────────────────────────────────────────
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushRange.offset     = 0;
    pushRange.size       = sizeof(float) * 16; // mat4

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount         = 1;
    layoutInfo.pSetLayouts            = &cfg.descriptorLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges    = &pushRange;

    if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_layout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create pipeline layout");

    // ── Graphics Pipeline ─────────────────────────────────────────────────────
    VkGraphicsPipelineCreateInfo pipeInfo{};
    pipeInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeInfo.stageCount          = 2;
    pipeInfo.pStages             = stages;
    pipeInfo.pVertexInputState   = &vertexInput;
    pipeInfo.pInputAssemblyState = &ia;
    pipeInfo.pViewportState      = &viewportState;
    pipeInfo.pRasterizationState = &raster;
    pipeInfo.pMultisampleState   = &ms;
    pipeInfo.pDepthStencilState  = &ds;
    pipeInfo.pColorBlendState    = &blend;
    pipeInfo.pDynamicState       = nullptr; // statisch
    pipeInfo.layout              = m_layout;
    pipeInfo.renderPass          = cfg.renderPass;
    pipeInfo.subpass             = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
                                  &pipeInfo, nullptr, &m_pipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create graphics pipeline");

    vkDestroyShaderModule(device, vertMod, nullptr);
    vkDestroyShaderModule(device, fragMod, nullptr);

    LOG_INFO("Graphics pipeline created");
}

// ── destroy ───────────────────────────────────────────────────────────────────
void Pipeline::destroy(VkDevice device) {
    vkDestroyPipeline      (device, m_pipeline, nullptr);
    vkDestroyPipelineLayout(device, m_layout,   nullptr);
}
