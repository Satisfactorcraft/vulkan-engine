#include "Window.hpp"
#include "Logger.hpp"
#include <stdexcept>

Window::Window(int width, int height, const std::string& title)
    : m_width(width), m_height(height)
{
    if (!glfwInit())
        throw std::runtime_error("GLFW init failed");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);  // kein OpenGL
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_window)
        throw std::runtime_error("GLFW window creation failed");

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);

    LOG_INFO("Window created: " << width << "x" << height);
}

Window::~Window() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool Window::shouldClose() const { return glfwWindowShouldClose(m_window); }
void Window::pollEvents() const  { glfwPollEvents(); }

void Window::framebufferResizeCallback(GLFWwindow* w, int width, int height) {
    auto* self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(w));
    self->m_resized = true;
    self->m_width   = width;
    self->m_height  = height;
    if (self->m_resizeCb) self->m_resizeCb(width, height);
}
