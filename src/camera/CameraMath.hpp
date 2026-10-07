#pragma once
#include <cmath>

namespace recoilexpand {

struct Quat { float x{}, y{}, z{}, w{1.f}; };

inline Quat quatFromEulerDeg(float pitch, float yaw, float roll) {
    const float d2r = 0.01745329251f;
    const float p = pitch * d2r * 0.5f;
    const float y = yaw * d2r * 0.5f;
    const float r = roll * d2r * 0.5f;
    const float sp = std::sin(p), cp = std::cos(p);
    const float sy = std::sin(y), cy = std::cos(y);
    const float sr = std::sin(r), cr = std::cos(r);
    Quat q;
    q.w = cr * cp * cy + sr * sp * sy;
    q.x = sr * cp * cy - cr * sp * sy;
    q.y = cr * sp * cy + sr * cp * sy;
    q.z = cr * cp * sy - sr * sp * cy;
    return q;
}

inline Quat quatMul(const Quat& a, const Quat& b) {
    return {
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
    };
}

// component+0x28 is camera orientation quat in CO (x,y,z,w floats)
inline void applyBiasToComponent(void* component, float pitchDeg, float yawDeg, float rollDeg) {
    if (!component) return;
    auto* q = reinterpret_cast<float*>(reinterpret_cast<char*>(component) + 0x28);
    Quat cur{q[0], q[1], q[2], q[3]};
    Quat bias = quatFromEulerDeg(pitchDeg, yawDeg, rollDeg);
    Quat out = quatMul(cur, bias);
    q[0] = out.x; q[1] = out.y; q[2] = out.z; q[3] = out.w;
}

} // namespace recoilexpand
