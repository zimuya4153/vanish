#pragma once

#include "Global.h"

#include <functional>
#include <ll/api/service/Bedrock.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>
#include <string>
#include <windows.h>

namespace PlaceholderAPI {
void registerServerPlaceholder(
    std::string const&           name,
    std::function<std::string()> callback,
    std::string const&           PluginName
) {
    auto func = (bool (*)(std::string const&, std::function<std::string()>, std::string const&))GetProcAddress(
        GetModuleHandle(L"GMLIB.dll"),
        "?registerServerPlaceholder@PlaceholderAPI@Server@GMLIB@@SA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$"
        "allocator@D@2@@std@@V?$function@$$A6A?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ@5@0@Z"
    );
    if (!func) return logger.error("“注册服务器PAPI变量函数” 获取失败，无法注册PAPI变量。");
    if (!func(name, callback, PluginName)) return logger.error("注册服务器PAPI变量 {0} 失败。", name);
}

void registerPlayerPlaceholder(
    std::string const&                  name,
    std::function<std::string(Player*)> callback,
    std::string const&                  PluginName
) {
    auto func = (bool (*)(std::string const&, std::function<std::string(Player*)>, std::string const&))GetProcAddress(
        GetModuleHandle(L"GMLIB.dll"),
        "?registerPlayerPlaceholder@PlaceholderAPI@Server@GMLIB@@SA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$"
        "allocator@D@2@@std@@V?$function@$$A6A?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@"
        "PEAVPlayer@@@Z@5@0@Z"
    );
    if (!func) return logger.error("“注册玩家PAPI变量函数” 获取失败，无法注册PAPI变量。");
    if (!func(name, callback, PluginName)) return logger.error("注册玩家PAPI变量 {0} 失败。", name);
}

void unregisterPlaceholder(std::string const& placeholder) {
    auto func = (bool (*)(std::string const&))GetProcAddress(
        GetModuleHandle(L"GMLIB.dll"),
        "?unregisterPlaceholder@PlaceholderAPI@Server@GMLIB@@SA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$"
        "allocator@D@2@@std@@@Z"
    );
    if (!func) return logger.error("“注销PAPI变量函数” 获取失败，无法注销PAPI变量。");
    if (!func(placeholder)) return logger.error("注销PAPI变量 {0} 失败。", placeholder);
}
} // namespace PlaceholderAPI

void PAPICall(bool enable) {
    if (enable) {
        PlaceholderAPI::registerServerPlaceholder(
            "vanish_vanishCount",
            []() -> std::string {
                int result = 0;
                if (auto level = ll::service::getLevel(); level)
                    level->forEachPlayer([&result](Player& player) -> bool {
                        if (config.playerConfigs[player.getUuid()].enabled) result++;
                        return true;
                    });
                return std::to_string(result);
            },
            "vanish"
        );
        PlaceholderAPI::registerServerPlaceholder(
            "vanish_invisibleCount",
            []() -> std::string {
                int result = 0;
                if (auto level = ll::service::getLevel(); level)
                    level->forEachPlayer([&result](Player& player) -> bool {
                        if (!config.playerConfigs[player.getUuid()].enabled) result++;
                        return true;
                    });
                return std::to_string(result);
            },
            "vanish"
        );
        PlaceholderAPI::registerPlayerPlaceholder(
            "vanish_isVanish",
            [](Player* player) -> std::string {
                return player && config.playerConfigs[player->getUuid()].enabled ? "是" : "否";
            },
            "vanish"
        );
    } else {
        // PlaceholderAPI::unregisterPlaceholder("vanish");   会导致无法卸载，咱也不知道为啥(・｀ω´・)
        PlaceholderAPI::unregisterPlaceholder("vanish_vanishCount");
        PlaceholderAPI::unregisterPlaceholder("vanish_invisibleCount");
        PlaceholderAPI::unregisterPlaceholder("vanish_isVanish");
    }
}