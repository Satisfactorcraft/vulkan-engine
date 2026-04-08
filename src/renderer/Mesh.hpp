#pragma once
#include "vulkan/VkContext.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <string>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 tangent;
};

class Mesh {
public:
    void loadOBJ(const VkContext& ctx, VkCommandPool pool, const std::string& path);
    void destroy(VkDevice device);
    void bind(VkCommandBuffer cmd) const;
    void draw(VkCommandBuffer cmd) const;

    uint32_t getIndexCount() const { return m_indexCount; }

private:
    void computeTangents(std::vector<Vertex>& verts, const std::vector<uint32_t>& indices);

    VkBuffer       m_vertexBuffer       = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer       m_indexBuffer        = VK_NULL_HANDLE;
    VkDeviceMemory m_indexBufferMemory  = VK_NULL_HANDLE;
    uint32_t       m_indexCount         = 0;
};
