cmake_minimum_required(VERSION 4.3)
project(VulkanEngine)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

add_executable(VulkanEngine
    src/main.cpp
    src/core/Window.cpp
    src/vulkan/VkContext.cpp
    src/vulkan/Swapchain.cpp
    src/vulkan/CommandPool.cpp
    src/vulkan/Buffer.cpp
    src/vulkan/Image.cpp
    src/vulkan/RenderPass.cpp
    src/vulkan/Descriptor.cpp
    src/vulkan/Pipeline.cpp
    src/renderer/Mesh.cpp
    src/renderer/Renderer.cpp
    src/scene/SceneLoader.cpp 
)

target_include_directories(VulkanEngine PRIVATE src external)

target_link_libraries(VulkanEngine PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
)
