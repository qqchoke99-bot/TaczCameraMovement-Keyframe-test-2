#pragma once
#include "camera/CurveLibrary.hpp"
#include "camera/KeyframeCurve.hpp"
#include <atomic>
#include <mutex>
#include <string>

namespace recoilexpand {

class RecoilModule {
public:
    static RecoilModule& get() {
        static RecoilModule inst;
        return inst;
    }

    static constexpr const char* moduleId = "recoilexpand.camera";
    static constexpr const char* name = "Recoil Expand";

    void init();
    void shutdown();

    void setEnabled(bool v) { m_enabled = v; }
    bool enabled() const { return m_enabled; }

    float globalIntensity() const { return m_globalIntensity; }
    void setGlobalIntensity(float v) { m_globalIntensity = v; }

    float inspectIntensity() const { return m_inspectIntensity; }
    void setInspectIntensity(float v) { m_inspectIntensity = v; }

    // Trigger from tags / external
    void triggerFire(const std::string& gunIdOrAlias, float intensityMul = 1.f);
    void triggerInspect(const std::string& style = "default");

    // Called every camera blend tick
    void onCameraBlend(void* component, float dt);

    CurveLibrary& library() { return m_lib; }

    // config
    bool m_enableRecoil{true};
    bool m_enableInspect{true};
    float m_globalIntensity{1.f};
    float m_inspectIntensity{1.f};
    float m_adsMultiplier{0.75f}; // when tag tacz_recoil_ads present
    std::string m_dataPath{"/sdcard/games/RecoilExpand"};

private:
    RecoilModule() = default;
    CurveLibrary m_lib;
    ActiveShot m_fire;
    ActiveShot m_inspect;
    std::mutex m_mu;
    bool m_enabled{true};
    bool m_inited{false};
};

} // namespace recoilexpand
