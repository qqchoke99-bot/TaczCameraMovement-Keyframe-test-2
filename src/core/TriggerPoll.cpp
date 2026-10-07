#include <filesystem>
#include "module/RecoilModule.hpp"
#include <android/log.h>
#include <fstream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "RecoilExpand", __VA_ARGS__)

// Side-channel triggers (BP / mcfunction / script writes these files):
//   /sdcard/games/RecoilExpand/trigger/fire.txt     content = gun id (e.g. krep_akm or ak47)
//   /sdcard/games/RecoilExpand/trigger/inspect.txt  content = style (default|rifle|pistol)
// File is deleted after consume.

namespace recoilexpand::trigger {

std::atomic<bool> g_run{false};
std::thread g_th;

void pollLoop() {
    namespace fs = std::filesystem;
    const fs::path firePath = "/sdcard/games/RecoilExpand/trigger/fire.txt";
    const fs::path inspectPath = "/sdcard/games/RecoilExpand/trigger/inspect.txt";
    fs::create_directories("/sdcard/games/RecoilExpand/trigger");

    while (g_run) {
        try {
            if (fs::exists(firePath)) {
                std::ifstream in(firePath);
                std::string id;
                std::getline(in, id);
                in.close();
                fs::remove(firePath);
                while (!id.empty() && (id.back() == '\r' || id.back() == '\n' || id.back() == ' '))
                    id.pop_back();
                if (!id.empty()) {
                    RecoilModule::get().triggerFire(id);
                }
            }
            if (fs::exists(inspectPath)) {
                std::ifstream in(inspectPath);
                std::string style;
                std::getline(in, style);
                in.close();
                fs::remove(inspectPath);
                while (!style.empty() && (style.back() == '\r' || style.back() == '\n' || style.back() == ' '))
                    style.pop_back();
                if (style.empty()) style = "default";
                RecoilModule::get().triggerInspect(style);
            }
        } catch (...) {
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }
}

void start() {
    if (g_run) return;
    g_run = true;
    g_th = std::thread(pollLoop);
    g_th.detach();
    LOGI("trigger poller started");
}

void stop() {
    g_run = false;
}

} // namespace recoilexpand::trigger
