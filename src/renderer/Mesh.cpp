#include "Mesh.hpp"
#include "vulkan/Buffer.hpp"
#include "core/Logger.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <unordered_map>
#include <stdexcept>
#include <glm/glm.hpp>

void Mesh::loadOBJ(const VkContext& ctx, VkCommandPool pool, const std::string& path) {
    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str()))
        throw std::runtime_error("TinyOBJ: " + warn + err);

    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;
    std::unordered_map<size_t, uint32_t> uniqueVerts;

    for (auto& shape : shapes) {
        for (auto& idx : shape.mesh.indices) {
            Vertex v{};
            v.pos = {
                attrib.vertices[3 * idx.vertex_index + 0],
                attrib.vertices[3 * idx.vertex_index + 1],
                attrib.vertices[3 * idx.vertex_index + 2]
            };
            if (idx.normal_index >= 0) {
                v.normal = {
                    attrib.normals[3 * idx.normal_index + 0],
                    attrib.normals[3 * idx.normal_index + 1],
                    attrib.normals[3 * idx.normal_index + 2]
                };
            }
            if (idx.texcoord_index >= 0) {
                v.uv = {
                    attrib.texcoords[2 * idx.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * idx.texcoord_index + 1]  // Y flip
                };
            }

            // Simple hash für Deduplication
            size_t hash = std::hash<float>{}(v.pos.x) ^
                          std::hash<float>{}(v.pos.y) ^
                          std::hash<float>{}(v.pos.z);
            auto it = uniqueVerts.find(hash);
            if (it == uniqueVerts.end()) {
                uniqueVerts[hash] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(v);
            }
            indices.push_back(uniqueVerts[hash]);
        }
    }

    computeTangents(vertices, indices);

    m_indexCount = static_cast<uint32_t>(indices.size());

    Buffer::createAndUpload(ctx, pool,
        vertices.data(), vertices.size() * sizeof(Vertex),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        m_vertexBuffer, m_vertexBufferMemory);

    Buffer::createAndUpload(ctx, pool,
        indices.data(), indices.size() * sizeof(uint32_t),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        m_indexBuffer, m_indexBufferMemory);

    LOG_INFO("Mesh loaded: " << path << "  verts=" << vertices.size()
             << "  tris=" << indices.size()/3);
}

void Mesh::computeTangents(std::vector<Vertex>& verts, const std::vector<uint32_t>& indices) {
    for (size_t i = 0; i < indices.size(); i += 3) {
        Vertex& v0 = verts[indices[i]];
        Vertex& v1 = verts[indices[i+1]];
        Vertex& v2 = verts[indices[i+2]];

        glm::vec3 e1  = v1.pos - v0.pos;
        glm::vec3 e2  = v2.pos - v0.pos;
        glm::vec2 du1 = v1.uv  - v0.uv;
        glm::vec2 du2 = v2.uv  - v0.uv;

        float f = 1.0f / (du1.x * du2.y - du2.x * du1.y + 1e-7f);
        glm::vec3 t = f * (du2.y * e1 - du1.y * e2);

        v0.tangent += t;
        v1.tangent += t;
        v2.tangent += t;
    }
    for (auto& v : verts)
        if (glm::length(v.tangent) > 0.0001f)
            v.tangent = glm::normalize(v.tangent);
}

void Mesh::destroy(VkDevice device) {
    vkDestroyBuffer(device, m_indexBuffer,        nullptr);
    vkFreeMemory   (device, m_indexBufferMemory,  nullptr);
    vkDestroyBuffer(device, m_vertexBuffer,       nullptr);
    vkFreeMemory   (device, m_vertexBufferMemory, nullptr);
}

void Mesh::bind(VkCommandBuffer cmd) const {
    VkBuffer     bufs[] = { m_vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, bufs, offsets);
    vkCmdBindIndexBuffer(cmd, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
}

void Mesh::draw(VkCommandBuffer cmd) const {
    vkCmdDrawIndexed(cmd, m_indexCount, 1, 0, 0, 0);
}
