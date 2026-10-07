#include "core/Hooks.hpp"
#include "core/TriggerPoll.hpp"
#include "module/RecoilModule.hpp"

#include <pl/Mod.hpp>
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "RecoilExpand", __VA_ARGS__)

class RecoilExpandMod : public pl::Mod {
public:
    bool load() override {
        LOGI("load");
        return true;
    }

    bool enable() override {
        LOGI("enable");
        auto& mod = recoilexpand::RecoilModule::get();
        mod.init();
        recoilexpand::hooks::install();
        recoilexpand::trigger::start();
        return true;
    }

    bool disable() override {
        LOGI("disable");
        recoilexpand::trigger::stop();
        recoilexpand::hooks::uninstall();
        recoilexpand::RecoilModule::get().shutdown();
        return true;
    }
};

PL_REGISTER_MOD(RecoilExpandMod)
