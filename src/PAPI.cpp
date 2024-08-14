#pragma once

#include "Global.h"

#include <functional>
#include <ll/api/service/Bedrock.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>
#include <string>
#include <windows.h>

void registerServerPAPI() {
    typedef bool (*RegisterServerPlaceholderFun)(
        std::string const&           name,
        std::function<std::string()> callback,
        std::string const&           PluginName
    );
    auto registerServerPlaceholder = (RegisterServerPlaceholderFun)GetProcAddress(
        GetModuleHandle(L"GMLIB.dll"),
        "?registerServerPlaceholder@PlaceholderAPI@Server@GMLIB@@SA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$"
        "allocator@D@2@@std@@V?$function@$$A6A?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ@5@0@Z"
    );
    if (!registerServerPlaceholder) return logger.warn("注册服务器PAPI变量函数获取失败，已停止注册。");
    if (!registerServerPlaceholder(
            "vanish_vanishCount",
            []() -> std::string {
                int result = 0;
                if (auto level = ll::service::getLevel()) {
                    level->forEachPlayer([&result](Player& player) -> bool {
                        if (config.playerConfigs[player.getUuid()].enabled) {
                            result++;
                        }
                        return true;
                    });
                }
                return std::to_string(result);
            },
            "vanish"
        ))
        logger.error("注册PAPI变量 vanish_vanishCount 失败。");
    if (!registerServerPlaceholder(
            "vanish_invisibleCount",
            []() -> std::string {
                int result = 0;
                if (auto level = ll::service::getLevel()) {
                    level->forEachPlayer([&result](Player& player) -> bool {
                        if (!config.playerConfigs[player.getUuid()].enabled) {
                            result++;
                        }
                        return true;
                    });
                }
                return std::to_string(result);
            },
            "vanish"
        ))
        logger.error("注册PAPI变量 vanish_invisibleCount 失败。");
}

void registerPlayerPAPI() {
    typedef bool (*RegisterPlayerPlaceholderFun)(
        std::string const&                        name,
        std::function<std::string(class Player*)> callback,
        std::string const&                        PluginName
    );
    auto registerServerPlaceholder = (RegisterPlayerPlaceholderFun)GetProcAddress(
        GetModuleHandle(L"GMLIB.dll"),
        "?registerPlayerPlaceholder@PlaceholderAPI@Server@GMLIB@@SA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$"
        "allocator@D@2@@std@@V?$function@$$A6A?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@"
        "PEAVPlayer@@@Z@5@0@Z"
    );
    if (!registerServerPlaceholder) return logger.warn("注册玩家PAPI变量函数获取失败，已停止注册。");
    if (!registerServerPlaceholder(
            "vanish_isVanish",
            [](Player* player) -> std::string { return config.playerConfigs[player->getUuid()].enabled ? "是" : "否"; },
            "vanish"
        ))
        logger.error("注册PAPI变量 vanish_isVanish 失败。");
}

void registerPAPI() {
    registerServerPAPI();
    registerPlayerPAPI();
}

void unregisterAllPAPI() {
    typedef bool (*UnregisterPlaceholderFun)(std::string const& placeholder);
    auto unregisterPlaceholder = (UnregisterPlaceholderFun)GetProcAddress(
        GetModuleHandle(L"GMLIB.dll"),
        "?unregisterPlaceholder@PlaceholderAPI@Server@GMLIB@@SA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$"
        "allocator@D@2@@std@@@Z"
    );
    if (!unregisterPlaceholder) return logger.error("注销PAPI变量函数获取失败，无法卸载PAPI变量。");
    unregisterPlaceholder("vanish_vanishCount");
    unregisterPlaceholder("vanish_invisibleCount");
    unregisterPlaceholder("vanish_isVanish");
}