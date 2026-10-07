#include "core/TriggerPoll.hpp"
#include "module/RecoilModule.hpp"

#include <android/log.h>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "RecoilExpand", __VA_ARGS__)

namespace recoilexpand::trigger {

namespace {
std::atomic<bool> g_run{false};
std::thread g_th;

std::string trim(std::string s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' ')) s.pop_back();
    return s;
}

void tryConsumeFire(const std::filesystem::path& firePath) {
    namespace fs = std::filesystem;
    if (!fs::exists(firePath)) return;
    std::ifstream in(firePath);
    std::string id;
    std::getline(in, id);
    in.close();
    try { fs::remove(firePath); } catch (...) {}
    id = trim(id);
    if (id.empty()) id = "test";
    LOGI("poll fire: %s", id.c_str());
    RecoilModule::get().triggerFire(id);
}

void tryConsumeInspect(const std::filesystem::path& path) {
    namespace fs = std::filesystem;
    if (!fs::exists(path)) return;
    std::ifstream in(path);
    std::string style;
    std::getline(in, style);
    in.close();
    try { fs::remove(path); } catch (...) {}
    style = trim(style);
    if (style.empty()) style = "default";
    LOGI("poll inspect: %s", style.c_str());
    RecoilModule::get().triggerInspect(style);
}

void pollLoop() {
    namespace fs = std::filesystem;
    const std::vector<fs::path> roots = {
        "/sdcard/games/RecoilExpand",
        "/storage/emulated/0/games/RecoilExpand",
        "/storage/emulated/0/Android/media/org.levimc.launcher/RecoilExpand",
        "/sdcard/Android/media/org.levimc.launcher/RecoilExpand",
    };
    for (const auto& r : roots) {
        try { fs::create_directories(r / "trigger"); } catch (...) {}
    }

    while (g_run.load()) {
        try {
            for (const auto& r : roots) {
                tryConsumeFire(r / "trigger" / "fire.txt");
                tryConsumeInspect(r / "trigger" / "inspect.txt");
            }
        } catch (...) {
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }
}
} // namespace

void start() {
    if (g_run.exchange(true)) return;
    g_th = std::thread(pollLoop);
    g_th.detach();
    LOGI("trigger poller started");
}

void stop() {
    g_run = false;
}

} // namespace recoilexpand::trigger
