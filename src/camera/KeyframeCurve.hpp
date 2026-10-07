#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace recoilexpand {

struct Keyframe {
    float time{};
    float lo{}; // random range low
    float hi{}; // random range high
};

struct AxisCurve {
    std::vector<Keyframe> keys;

    // Sample degrees at time t (loop clamp). Uses midpoint of range (stable).
    float sample(float t) const {
        if (keys.empty()) return 0.f;
        if (t <= keys.front().time) return 0.5f * (keys.front().lo + keys.front().hi);
        if (t >= keys.back().time) return 0.5f * (keys.back().lo + keys.back().hi);
        for (size_t i = 0; i + 1 < keys.size(); ++i) {
            const auto& a = keys[i];
            const auto& b = keys[i + 1];
            if (t >= a.time && t <= b.time) {
                const float u = (b.time > a.time) ? (t - a.time) / (b.time - a.time) : 0.f;
                const float av = 0.5f * (a.lo + a.hi);
                const float bv = 0.5f * (b.lo + b.hi);
                // smoothstep
                const float s = u * u * (3.f - 2.f * u);
                return av + (bv - av) * s;
            }
        }
        return 0.f;
    }

    float duration() const { return keys.empty() ? 0.f : keys.back().time; }
};

struct CameraCurve {
    AxisCurve pitch;
    AxisCurve yaw;
    AxisCurve roll;
    float duration() const {
        return std::max({pitch.duration(), yaw.duration(), roll.duration()});
    }
};

struct ActiveShot {
    CameraCurve curve;
    float t{0.f};
    float intensity{1.f};
    bool alive{false};

    void start(const CameraCurve& c, float inten = 1.f) {
        curve = c;
        t = 0.f;
        intensity = inten;
        alive = true;
    }

    // returns pitch,yaw,roll degrees this frame; advances time
    std::tuple<float, float, float> tick(float dt) {
        if (!alive) return {0, 0, 0};
        t += dt;
        if (t >= curve.duration()) {
            alive = false;
            return {0, 0, 0};
        }
        return {
            curve.pitch.sample(t) * intensity,
            curve.yaw.sample(t) * intensity,
            curve.roll.sample(t) * intensity,
        };
    }
};

} // namespace recoilexpand
