#pragma once

#include "Entry.h"
#include "Global.h"

#include <ll/api/Config.h>
#include <ll/api/mod/RegisterHelper.h>
#include <memory>

Vanish::Config config;
ll::Logger     logger("vanish");

namespace Vanish {

static std::unique_ptr<Entry> instance;

Entry& Entry::getInstance() { return *instance; }

bool Entry::load() {
    auto path = getSelf().getConfigDir() / "config.json";
    try {
        ll::config::loadConfig(config, path);
        auto                   defaultConfig = Vanish::PlayerConfig();
        std::vector<mce::UUID> removes;
        for (auto& item : config.playerConfigs) {
            if (item.second == defaultConfig) removes.push_back(item.first);
        }
        for (auto& item : removes) config.playerConfigs.erase(item);
    } catch (...) {}
    ll::config::saveConfig(config, path);
    logger.info(fmt::format(
        fmt::fg(fmt::color::pink),
        "模组 vanish-v{} 已加载！作者: 小小的子沐呀 QQ:1756150362",
        VERSION.to_string()
    ));
    return true;
}

bool Entry::enable() {
    registerCommands();
    HookCall(true);
    if (config.enabledPAPI) PapiCall(true);
    logger.info(fmt::format(
        fmt::fg(fmt::color::aqua),
        "欢迎使用隐身模组，如有问题，请加Q群(985991178)反馈或前往github提交issues。"
    ));
    logger.info(fmt::format(fmt::fg(fmt::color::yellow), "Github地址: {}", "https://github.com/zimuya4153/vanish"));
    logger.info(fmt::format(fmt::fg(fmt::color::red), "点个star支持一下吧QvQ 喵喵喵……QvQ"));
    return true;
}

bool Entry::disable() {
    if (config.enabledPAPI) PapiCall(false);
    HookCall(false);
    return true;
}

bool Entry::unload() { return true; }

} // namespace Vanish

LL_REGISTER_MOD(Vanish::Entry, Vanish::instance);