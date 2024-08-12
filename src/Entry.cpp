#pragma once

#include "Entry.h"
#include "Global.h"

#include <ll/api/Config.h>
#include <ll/api/mod/RegisterHelper.h>
#include <memory>

ll::Logger     logger("vanish");
Vanish::Config config;

namespace Vanish {

static std::unique_ptr<Entry> instance;

Entry& Entry::getInstance() { return *instance; }

bool Entry::load() {
    auto path = getSelf().getConfigDir() / "config.json";
    try {
        if (!ll::config::loadConfig(config, path)) {
            ll::config::saveConfig(config, path);
        }
    } catch (...) {
        ll::config::saveConfig(config, path);
    }
    return true;
}

bool Entry::enable() {
    registerCommands();
    if (config.enabledPAPI) registerPAPI();
    logger.info(
        fmt::format(fg(fmt::color::aqua), "欢迎使用隐身插件，如有问题，请加Q群(985991178)反馈或前往github提交issues。")
    );
    logger.info(fmt::format(fg(fmt::color::yellow), "Github地址: {}", "https://github.com/zimuya4153/vanish"));
    return true;
}

bool Entry::disable() { return true; }

bool Entry::unload() {
    if (config.enabledPAPI) unregisterAllPAPI();
    return true;
}

} // namespace Vanish

LL_REGISTER_MOD(Vanish::Entry, Vanish::instance);