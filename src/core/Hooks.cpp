#include "core/Hooks.hpp"
#include "module/RecoilModule.hpp"

#include <pl/memory/Hook.hpp>
#include <pl/memory/Signature.hpp>

#include <android/log.h>
#include <atomic>
#include <chrono>
#include <string>
#include <vector>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "RecoilExpand", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "RecoilExpand", __VA_ARGS__)

namespace {

// Patterns used by CameraOverhaul family (1.26.50+) — try several
const char* kPatterns[] = {
    // CO Natural-Camera CameraBlendSystemTick
    "? ? ? D1 ? ? ? 6D ? ? ? 6D ? ? ? 6D ? ? ? 6D ? ? ? A9 ? ? ? F9 ? ? ? A9 ? ? ? A9 ? ? ? 91 "
    "55 D0 3B D5 F3 03 01 AA F4 03 00 AA ? ? ? F9 ? ? ? 91 ? ? ? 91",
    // Alternate AOB from earlier RE session
    "F4 4F 06 A9 FD 03 01 91 54 D0 3B D5 F3 03 00 AA 88 ?? ?? F9 A8 83 1F F8 "
    "?? ?? ?? D0 08 A1 11 91 08 FD DF 08 ?? ?? ?? ?? ?? ?? ?? D0 21 E0 0C 91 "
    "E0 63 00 91 ?? ?? ?? 95",
};

using CameraBlendFn = void (*)(void* component, void* blendState, float factor);
CameraBlendFn g_origCameraBlend = nullptr;

std::chrono::steady_clock::time_point g_last{};
std::atomic<bool> g_hooked{false};

void hookedCameraBlend(void* component, void* blendState, float factor) {
    if (g_origCameraBlend) {
        g_origCameraBlend(component, blendState, factor);
    }

    using clock = std::chrono::steady_clock;
    const auto now = clock::now();
    float dt = 1.f / 60.f;
    if (g_last.time_since_epoch().count() != 0) {
        dt = std::chrono::duration<float>(now - g_last).count();
        if (dt < 0.001f) dt = 0.001f;
        if (dt > 0.1f) dt = 0.1f;
    }
    g_last = now;

    recoilexpand::RecoilModule::get().onCameraBlend(component, dt);
}

} // namespace

namespace recoilexpand::hooks {

bool install() {
    if (g_hooked.load()) return true;

    std::vector<std::string> patterns;
    for (const char* p : kPatterns) patterns.emplace_back(p);

    const auto resolved = pl::memory::resolveSignatures(patterns, "libminecraftpe.so");

    void* target = nullptr;
    for (const auto& pat : patterns) {
        const auto it = resolved.find(pat);
        if (it != resolved.end() && it->second != 0) {
            target = reinterpret_cast<void*>(it->second);
            LOGI("signature matched, addr=%p", target);
            break;
        }
    }

    if (!target) {
        LOGE("NO CameraBlend signature matched — camera recoil disabled");
        return false;
    }

    const int rc = pl::memory::hook(
        target,
        reinterpret_cast<void*>(hookedCameraBlend),
        reinterpret_cast<void**>(&g_origCameraBlend));

    if (rc != 0 || g_origCameraBlend == nullptr) {
        LOGE("hook failed rc=%d orig=%p", rc, (void*)g_origCameraBlend);
        return false;
    }

    g_hooked = true;
    LOGI("CameraBlend HOOKED OK @ %p", target);
    return true;
}

void uninstall() {
    g_hooked = false;
}

} // namespace recoilexpand::hooks
