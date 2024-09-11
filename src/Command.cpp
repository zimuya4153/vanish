#pragma once

#include "Entry.h"
#include "Global.h"

#include <ll/api/Config.h>
#include <ll/api/command/CommandHandle.h>
#include <ll/api/command/CommandRegistrar.h>
#include <ll/api/form/CustomForm.h>
#include <mc/entity/utilities/ActorType.h>
#include <mc/server/commands/CommandOrigin.h>
#include <mc/server/commands/CommandOutput.h>
#include <mc/server/commands/CommandPermissionLevel.h>
#include <mc/server/commands/CommandSelector.h>

void registerCommands() {
    auto& cmd = ll::command::CommandRegistrar::getInstance().getOrCreateCommand(
        config.command,
        "隐身",
        config.permMode ? CommandPermissionLevel::Any : CommandPermissionLevel::GameDirectors
    );
    if (!config.alias.empty()) cmd.alias(config.alias);
    struct CommandParam {
        enum action { add, remove } action;
        CommandSelector<Player> player;
    };
    cmd.overload().execute([&](CommandOrigin const& origin, CommandOutput& output) -> void {
        auto* entity = origin.getEntity();
        if (entity == nullptr || !entity->isType(ActorType::Player)) return output.error("非玩家不可执行");
        if (config.permMode
            && std::count(config.permPlayers.begin(), config.permPlayers.end(), static_cast<Player*>(entity)->getUuid())
                   == 0)
            return output.error("您没有权限使用该命令");
        auto* player       = static_cast<Player*>(entity);
        auto& playerConfig = config.playerConfigs[player->getUuid()];
        auto  form         = ll::form::CustomForm();
        form.setTitle("隐身菜单");
        form.appendLabel(fmt::format(
            "§6欢迎使用隐身插件 §c当前版本§d:§e{0}\n§b作者 §4小小的子沐呀\n§g--------------------------------",
            VERSION.to_string()
        ));
        form.appendToggle("enabled", "是否开启隐身", playerConfig.enabled);
        form.appendToggle("vanishPromptExit", "隐身后提示退出游戏", playerConfig.vanishPromptExit);
        form.appendToggle("appearPromptJoin", "解除隐身后提示加入游戏", playerConfig.appearPromptJoin);
        form.appendToggle("vanishBossbar", "隐身后显示Boss栏", playerConfig.vanishBossbar);
        form.appendInput(
            "vanishBossbarText",
            "隐身Boss栏文本",
            "您如果不要可以关闭哦~没必要留空",
            playerConfig.vanishBossbarText
        );
        form.appendToggle("vanishNoTakeItem", "隐身后禁止拾取物品", playerConfig.vanishNoTakeItem);
        form.appendToggle("vanishNoDropItem", "隐身后禁止丢出物品", playerConfig.vanishNoDropItem);
        form.appendToggle("vanishNoRedstone", "隐身后禁止触发红石机关", playerConfig.vanishNoRedstone);
        form.appendToggle("vanishNoTouchEntity", "隐身后禁止触碰实体", playerConfig.vanishNoTouchEntity);
        form.appendToggle("vanishNoInteractSound", "隐身后无交互方块音效", playerConfig.vanishNoInteractSound);
        form.sendTo(
            *player,
            [](Player& player, ll::form::CustomFormResult const& result, ll::form::FormCancelReason) -> void {
                if (!result.has_value()) return;
                setPlayerVanishConfig(
                    const_cast<mce::UUID&>(player.getUuid()),
                    Vanish::PlayerConfig{
                        static_cast<bool>(std::get<uint64>(result->at("enabled"))),
                        static_cast<bool>(std::get<uint64>(result->at("appearPromptJoin"))),
                        static_cast<bool>(std::get<uint64>(result->at("vanishPromptExit"))),
                        static_cast<bool>(std::get<uint64>(result->at("vanishBossbar"))),
                        std::get<std::string>(result->at("vanishBossbarText")),
                        static_cast<bool>(std::get<uint64>(result->at("vanishNoTakeItem"))),
                        static_cast<bool>(std::get<uint64>(result->at("vanishNoDropItem"))),
                        static_cast<bool>(std::get<uint64>(result->at("vanishNoRedstone"))),
                        static_cast<bool>(std::get<uint64>(result->at("vanishNoTouchEntity"))),
                        static_cast<bool>(std::get<uint64>(result->at("vanishNoInteractSound"))),
                    }
                );
            }
        );
    });
    if (config.permMode)
        cmd.overload<CommandParam>().required("action").required("player").execute(
            [&](CommandOrigin const& origin, CommandOutput& output, CommandParam const& result) -> void {
                if (origin.getOriginType() != CommandOriginType::DedicatedServer)
                    return output.error("非控制台不可添加/删除玩家权限。");
                if (result.player.results(origin).empty()) return output.error("未选择玩家。");
                for (auto* player : result.player.results(origin)) {
                    switch (result.action) {
                    case result.add: {
                        if (std::count(config.permPlayers.begin(), config.permPlayers.end(), player->getUuid()) != 0) {
                            output.error("玩家 {} 已拥有权限。", player->getRealName());
                        } else {
                            config.permPlayers.push_back(player->getUuid());
                            output.success("玩家 {} 添加权限成功。", player->getRealName());
                        }
                        break;
                    }
                    case result.remove: {
                        if (std::count(config.permPlayers.begin(), config.permPlayers.end(), player->getUuid()) == 0) {
                            output.error("玩家 {} 本没有权限。", player->getRealName());
                        } else {
                            config.permPlayers.erase(
                                std::remove(config.permPlayers.begin(), config.permPlayers.end(), player->getUuid()),
                                config.permPlayers.end()
                            );
                            output.success("玩家 {} 删除权限成功。", player->getRealName());
                        }
                        break;
                    }
                    }
                }
                ll::config::saveConfig(config, Vanish::Entry::getInstance().getSelf().getConfigDir() / "config.json");
            }
        );
}