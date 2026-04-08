#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <optional>
#include <string>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR        capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;
};

class VkContext {
public:
    void init(GLFWwindow* window);
    void destroy();

    VkInstance               getInstance()       const { return m_instance; }
    VkPhysicalDevice         getPhysicalDevice() const { return m_physicalDevice; }
    VkDevice                 getDevice()         const { return m_device; }
    VkQueue                  getGraphicsQueue()  const { return m_graphicsQueue; }
    VkQueue                  getPresentQueue()   const { return m_presentQueue; }
    VkSurfaceKHR             getSurface()        const { return m_surface; }
    QueueFamilyIndices       getQueueFamilies()  const { return m_queueFamilies; }

    // Nur nach init() aufrufen
    SwapchainSupportDetails  querySwapchainSupport() const;

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props) const;
    VkFormat findDepthFormat() const;

private:
    void createInstance();
    void setupDebugMessenger();
    void createSurface(GLFWwindow* window);
    void pickPhysicalDevice();
    void createLogicalDevice();

    bool isDeviceSuitable(VkPhysicalDevice device) const;
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;
    bool checkValidationLayerSupport() const;
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;

    // Intern: device als Parameter → kein m_physicalDevice nötig
    SwapchainSupportDetails querySwapchainSupportForDevice(VkPhysicalDevice device) const;

    VkInstance               m_instance       = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR             m_surface        = VK_NULL_HANDLE;
    VkPhysicalDevice         m_physicalDevice = VK_NULL_HANDLE;
    VkDevice                 m_device         = VK_NULL_HANDLE;
    VkQueue                  m_graphicsQueue  = VK_NULL_HANDLE;
    VkQueue                  m_presentQueue   = VK_NULL_HANDLE;
    QueueFamilyIndices       m_queueFamilies;

    const std::vector<const char*> m_deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    const std::vector<const char*> m_validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
#ifdef NDEBUG
    const bool m_enableValidation = false;
#else
    const bool m_enableValidation = true;
#endif
};
