#pragma once
#include "camera/KeyframeCurve.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

namespace recoilexpand {

inline AxisCurve parseAxis(const nlohmann::json& arr) {
    AxisCurve c;
    if (!arr.is_array()) return c;
    for (auto& kf : arr) {
        Keyframe k;
        k.time = kf.value("time", 0.f);
        auto v = kf.at("value");
        if (v.is_array() && v.size() >= 2) {
            k.lo = v[0].get<float>();
            k.hi = v[1].get<float>();
        } else if (v.is_number()) {
            k.lo = k.hi = v.get<float>();
        }
        c.keys.push_back(k);
    }
    return c;
}

inline CameraCurve parseCurve(const nlohmann::json& j) {
    CameraCurve c;
    if (j.contains("pitch")) c.pitch = parseAxis(j["pitch"]);
    if (j.contains("yaw")) c.yaw = parseAxis(j["yaw"]);
    if (j.contains("roll")) c.roll = parseAxis(j["roll"]);
    return c;
}

struct CurveLibrary {
    std::unordered_map<std::string, CameraCurve> recoilById;
    std::unordered_map<std::string, std::string> aliasToId;
    std::unordered_map<std::string, CameraCurve> inspectByStyle;
    CameraCurve defaultInspect;

    void loadGunJson(const nlohmann::json& j) {
        const std::string id = j.value("id", "");
        if (id.empty() || !j.contains("recoil")) return;
        recoilById[id] = parseCurve(j["recoil"]);
        for (auto& a : j.value("aliases", nlohmann::json::array())) {
            aliasToId[a.get<std::string>()] = id;
        }
        aliasToId[id] = id;
    }

    void loadInspectJson(const nlohmann::json& j) {
        for (auto it = j.begin(); it != j.end(); ++it) {
            inspectByStyle[it.key()] = parseCurve(it.value());
        }
        if (inspectByStyle.count("default"))
            defaultInspect = inspectByStyle["default"];
    }

    const CameraCurve* findRecoil(const std::string& gunOrAlias) const {
        auto a = aliasToId.find(gunOrAlias);
        if (a == aliasToId.end()) return nullptr;
        auto it = recoilById.find(a->second);
        return it == recoilById.end() ? nullptr : &it->second;
    }

    const CameraCurve& inspect(const std::string& style) const {
        auto it = inspectByStyle.find(style);
        if (it != inspectByStyle.end()) return it->second;
        return defaultInspect;
    }
};

} // namespace recoilexpand
