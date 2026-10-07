#include "module/RecoilModule.hpp"
#include "camera/CameraMath.hpp"

#include <android/log.h>
#include <filesystem>
#include <fstream>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "RecoilExpand", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "RecoilExpand", __VA_ARGS__)

namespace fs = std::filesystem;
namespace recoilexpand {

void RecoilModule::init() {
    if (m_inited) return;
    m_inited = true;

    // Load bundled-style data from /sdcard (user copies data/ there) or try relative
    const fs::path root(m_dataPath);
    const fs::path gunsDir = root / "data" / "guns";
    const fs::path inspectFile = root / "data" / "inspect" / "curves.json";

    int loaded = 0;
    if (fs::exists(gunsDir)) {
        for (auto& e : fs::directory_iterator(gunsDir)) {
            if (!e.is_regular_file() || e.path().extension() != ".json") continue;
            if (e.path().filename() == "index.json") continue;
            try {
                std::ifstream in(e.path());
                nlohmann::json j;
                in >> j;
                m_lib.loadGunJson(j);
                ++loaded;
            } catch (...) {
                LOGE("failed gun json %s", e.path().c_str());
            }
        }
    }
    if (fs::exists(inspectFile)) {
        try {
            std::ifstream in(inspectFile);
            nlohmann::json j;
            in >> j;
            m_lib.loadInspectJson(j);
            LOGI("inspect curves loaded");
        } catch (...) {
            LOGE("failed inspect curves");
        }
    }
    LOGI("RecoilExpand init: %d gun curves from %s", loaded, gunsDir.c_str());
}

void RecoilModule::shutdown() {
    std::lock_guard lk(m_mu);
    m_fire.alive = false;
    m_inspect.alive = false;
}

void RecoilModule::triggerFire(const std::string& gunIdOrAlias, float intensityMul) {
    if (!m_enableRecoil || !m_enabled) return;
    const CameraCurve* c = m_lib.findRecoil(gunIdOrAlias);
    if (!c) {
        // fallback: try "ak47" style default medium
        c = m_lib.findRecoil("ak47");
        if (!c) return;
    }
    std::lock_guard lk(m_mu);
    m_fire.start(*c, m_globalIntensity * intensityMul);
}

void RecoilModule::triggerInspect(const std::string& style) {
    if (!m_enableInspect || !m_enabled) return;
    std::lock_guard lk(m_mu);
    m_inspect.start(m_lib.inspect(style), m_inspectIntensity);
}

void RecoilModule::onCameraBlend(void* component, float dt) {
    if (!m_enabled || !component) return;

    float pitch = 0.f, yaw = 0.f, roll = 0.f;
    {
        std::lock_guard lk(m_mu);
        if (m_fire.alive) {
            auto [p, y, r] = m_fire.tick(dt);
            pitch += p;
            yaw += y;
            roll += r;
        }
        if (m_inspect.alive) {
            auto [p, y, r] = m_inspect.tick(dt);
            pitch += p;
            yaw += y;
            roll += r;
        }
    }
    if (pitch == 0.f && yaw == 0.f && roll == 0.f) return;
    applyBiasToComponent(component, pitch, yaw, roll);
}

} // namespace recoilexpand
