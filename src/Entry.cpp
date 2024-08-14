#pragma once

#include "Entry.h"
#include "Global.h"

#include <ll/api/Config.h>
#include <ll/api/mod/RegisterHelper.h>
#include <memory>

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
    logger.info(fmt::format(fmt::fg(fmt::color::pink), "插件已加载   版本:v{}", VERSION.to_string()));
    logger.debug(fmt::format(fmt::fg(fmt::color::light_green), "[load] 喵~"));
    return true;
}

bool Entry::enable() {
    registerCommands();
    loadAllHook();
    if (config.enabledPAPI) registerPAPI();
    logger.info(fmt::format(
        fmt::fg(fmt::color::aqua),
        "欢迎使用隐身插件，如有问题，请加Q群(985991178)反馈或前往github提交issues。"
    ));
    logger.info(fmt::format(fmt::fg(fmt::color::yellow), "Github地址: {}", "https://github.com/zimuya4153/vanish"));
    logger.debug(fmt::format(fmt::fg(fmt::color::light_green), "[enable] 隐身插件启动，嗷呜~~~"));
    return true;
}

bool Entry::disable() {
    logger.debug(fmt::format(fmt::fg(fmt::color::light_green), "[disable] 唔姆姆~"));
    if (config.enabledPAPI) unregisterAllPAPI();
    unloadAllHook();
    return true;
}

bool Entry::unload() {
    logger.debug(fmt::format(fmt::fg(fmt::color::light_green), "[unloda] 噫呜呜呜~"));
    if (config.enabledPAPI) unregisterAllPAPI();
    unloadAllHook();
    return true;
}

} // namespace Vanish

LL_REGISTER_MOD(Vanish::Entry, Vanish::instance);