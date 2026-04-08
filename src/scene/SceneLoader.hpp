#pragma once
#include "renderer/Renderer.hpp"
#include <string>

class SceneLoader {
public:
    static void load(const std::string& path,
                     Renderer&          renderer,
                     VkContext&         ctx,
                     CommandPool&       cmdPool);
};
