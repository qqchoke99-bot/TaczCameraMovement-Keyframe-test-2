#include "module/RecoilModule.hpp"
#include "camera/CameraMath.hpp"

#include <android/log.h>
#include <filesystem>
#include <fstream>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "RecoilExpand", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "RecoilExpand", __VA_ARGS__)

namespace fs = std::filesystem;
namespace recoilexpand {

namespace {

CameraCurve makeTestKick() {
    // Strong obvious kick so user can see if hook works
    CameraCurve c;
    c.pitch.keys = {
        {0.00f, 4.0f, 4.5f},
        {0.05f, 5.0f, 5.5f},
        {0.15f, 2.0f, 2.5f},
        {0.35f, -0.5f, -0.3f},
        {0.50f, 0.0f, 0.0f},
    };
    c.yaw.keys = {
        {0.00f, -1.5f, 1.5f},
        {0.20f, -0.5f, 0.5f},
        {0.50f, 0.0f, 0.0f},
    };
    c.roll.keys = {
        {0.00f, -1.0f, 1.0f},
        {0.50f, 0.0f, 0.0f},
    };
    return c;
}

} // namespace

void RecoilModule::init() {
    if (m_inited) return;
    m_inited = true;

    // Prefer games path; also try Levi media path
    const fs::path candidates[] = {
        fs::path("/sdcard/games/RecoilExpand"),
        fs::path("/storage/emulated/0/games/RecoilExpand"),
        fs::path("/storage/emulated/0/Android/media/org.levimc.launcher/RecoilExpand"),
        fs::path("/sdcard/Android/media/org.levimc.launcher/RecoilExpand"),
    };

    fs::path root;
    for (const auto& c : candidates) {
        if (fs::exists(c / "data" / "guns") || fs::exists(c / "data" / "config.json")) {
            root = c;
            m_dataPath = c.string();
            LOGI("data root: %s", m_dataPath.c_str());
            break;
        }
    }
    if (root.empty()) {
        root = candidates[0];
        m_dataPath = root.string();
        LOGE("data folder not found, defaulting to %s", m_dataPath.c_str());
    }

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
            } catch (const std::exception& ex) {
                LOGE("gun json fail %s: %s", e.path().c_str(), ex.what());
            } catch (...) {
                LOGE("gun json fail %s", e.path().c_str());
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
            LOGE("inspect curves failed");
        }
    }

    // Always have a test curve
    m_lib.recoilById["__test__"] = makeTestKick();
    m_lib.aliasToId["test"] = "__test__";
    m_lib.aliasToId["__test__"] = "__test__";

    LOGI("init done: %d gun curves, intensity=%.2f", loaded, m_globalIntensity);
}

void RecoilModule::shutdown() {
    std::lock_guard lk(m_mu);
    m_fire.alive = false;
    m_inspect.alive = false;
}

void RecoilModule::triggerFire(const std::string& gunIdOrAlias, float intensityMul) {
    if (!m_enableRecoil || !m_enabled) {
        LOGI("triggerFire ignored (disabled)");
        return;
    }
    const CameraCurve* c = m_lib.findRecoil(gunIdOrAlias);
    if (!c) {
        LOGI("no curve for '%s', using TEST kick", gunIdOrAlias.c_str());
        c = m_lib.findRecoil("__test__");
    }
    if (!c) return;
    std::lock_guard lk(m_mu);
    m_fire.start(*c, m_globalIntensity * intensityMul);
    LOGI("fire start id=%s inten=%.2f dur=%.2f", gunIdOrAlias.c_str(),
         m_globalIntensity * intensityMul, c->duration());
}

void RecoilModule::triggerInspect(const std::string& style) {
    if (!m_enableInspect || !m_enabled) return;
    std::lock_guard lk(m_mu);
    m_inspect.start(m_lib.inspect(style), m_inspectIntensity);
    LOGI("inspect start style=%s", style.c_str());
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
