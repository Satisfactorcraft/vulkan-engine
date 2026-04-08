#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <functional>

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    bool shouldClose() const;
    void pollEvents() const;

    GLFWwindow* getHandle() const { return m_window; }
    int getWidth()  const { return m_width; }
    int getHeight() const { return m_height; }
    bool wasResized() const { return m_resized; }
    void resetResizedFlag() { m_resized = false; }

    void setResizeCallback(std::function<void(int,int)> cb) { m_resizeCb = cb; }

private:
    static void framebufferResizeCallback(GLFWwindow* w, int width, int height);

    GLFWwindow* m_window = nullptr;
    int m_width, m_height;
    bool m_resized = false;
    std::function<void(int,int)> m_resizeCb;
};
