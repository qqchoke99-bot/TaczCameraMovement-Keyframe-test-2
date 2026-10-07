#include "module/RecoilModule.hpp"

#include <pl/memory/Hook.hpp>
#include <pl/memory/Signature.hpp>
#include <android/log.h>
#include <atomic>
#include <chrono>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "RecoilExpand", __VA_ARGS__)

namespace {

// CameraBlendSystemTick — same AOB used by CameraOverhaul / Natural-Camera 1.26.50+
// F4 4F 06 A9 FD 03 01 91 54 D0 3B D5 ... (fn Multiplayer camera systems path)
// Prefer resolve via string-backed sig if available; fallback pattern below.

using CameraBlendFn = void (*)(void* component, void* blendState, float factor);
CameraBlendFn g_origCameraBlend = nullptr;

std::chrono::steady_clock::time_point g_last;
std::atomic<bool> g_hooked{false};

void hookedCameraBlend(void* component, void* blendState, float factor) {
    if (g_origCameraBlend) g_origCameraBlend(component, blendState, factor);

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

// Optional: detect scoreboard/tag via command is done from BP.
// Native side also exposes a simple test: holding is not required for first version.

} // namespace

namespace recoilexpand::hooks {

bool install() {
    // Signature from CameraOverhaul CameraBlendSystemTick (1.26.50 family)
    static const char* kSig =
        "F4 4F 06 A9 FD 03 01 91 54 D0 3B D5 F3 03 00 AA 88 ?? ?? F9 A8 83 1F F8 "
        "?? ?? ?? D0 08 A1 11 91 08 FD DF 08 ?? ?? ?? ?? ?? ?? ?? D0 21 E0 0C 91 "
        "E0 63 00 91 ?? ?? ?? 95";

    auto resolved = pl::memory::resolveSignature(kSig);
    if (!resolved) {
        LOGI("CameraBlend signature not found — recoil camera hook skipped");
        return false;
    }

    const bool ok = pl::memory::hook(
        resolved,
        reinterpret_cast<void*>(hookedCameraBlend),
        reinterpret_cast<void**>(&g_origCameraBlend),
        pl::memory::HookPriority::Normal);

    g_hooked = ok;
    LOGI("CameraBlend hook %s", ok ? "OK" : "FAIL");
    return ok;
}

void uninstall() {
    // preloader unhooks on unload in most builds
    g_hooked = false;
}

} // namespace recoilexpand::hooks
