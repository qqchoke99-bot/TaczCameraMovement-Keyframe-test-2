#include "core/Hooks.hpp"
#include "core/TriggerPoll.hpp"
#include "module/RecoilModule.hpp"

#include <pl/Mod.hpp>
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "RecoilExpand", __VA_ARGS__)

class RecoilExpandMod {
public:
    static RecoilExpandMod& instance() {
        static RecoilExpandMod mod;
        return mod;
    }

    bool load(pl::mod::ModContext&) {
        LOGI("load");
        return true;
    }

    bool enable(pl::mod::ModContext&) {
        LOGI("enable");
        recoilexpand::RecoilModule::get().init();
        recoilexpand::hooks::install();
        recoilexpand::trigger::start();
        return true;
    }

    bool disable(pl::mod::ModContext&) {
        LOGI("disable");
        recoilexpand::trigger::stop();
        recoilexpand::hooks::uninstall();
        recoilexpand::RecoilModule::get().shutdown();
        return true;
    }

    bool unload(pl::mod::ModContext&) {
        LOGI("unload");
        return true;
    }
};

PL_REGISTER_MOD(RecoilExpandMod, RecoilExpandMod::instance())
