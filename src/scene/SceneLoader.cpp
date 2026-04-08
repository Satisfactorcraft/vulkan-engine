#include "SceneLoader.hpp"
#include "core/Logger.hpp"
#include <json.hpp>
#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

void SceneLoader::load(const std::string& path,
                       Renderer&          renderer,
                       VkContext&         ctx,
                       CommandPool&       cmdPool)
{
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Failed to open scene file: " + path);

    json j;
    file >> j;

    // ── Mesh + Texturen ───────────────────────────────────────────────────────
    std::string mesh       = j.at("mesh");
    std::string albedo     = j["textures"].at("albedo");
    std::string normal     = j["textures"].at("normal");
    std::string metalRough = j["textures"].at("metalRough");
    std::string ao         = j["textures"].at("ao");

    renderer.loadScene(ctx, cmdPool, mesh, albedo, normal, metalRough, ao);

    // ── Camera ────────────────────────────────────────────────────────────────
    if (j.contains("camera")) {
        auto& cam = j["camera"];

        if (cam.contains("position")) {
            auto p = cam["position"];
            renderer.camera.position = { p[0], p[1], p[2] };
        }
        if (cam.contains("target")) {
            auto t = cam["target"];
            renderer.camera.target = { t[0], t[1], t[2] };
        }
        if (cam.contains("fov"))
            renderer.camera.fovDeg = cam["fov"];
    }

    // ── Lights ────────────────────────────────────────────────────────────────
    if (j.contains("lights")) {
        auto& lights = j["lights"];
        int count = std::min((int)lights.size(), 4);
        renderer.numLights = count;

        for (int i = 0; i < count; ++i) {
            auto& l = lights[i];

            glm::vec3 pos   = { l["position"][0], l["position"][1], l["position"][2] };
            glm::vec3 color = { l["color"][0],    l["color"][1],    l["color"][2] };
            float intensity = l.value("intensity", 1.0f);

            renderer.setLight(i, pos, color, intensity);
        }
    }

    LOG_INFO("Scene loaded from: " << path);
}
