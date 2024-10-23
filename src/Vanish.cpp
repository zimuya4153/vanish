#pragma once

#include "Entry.h"
#include "Global.h"

#include <ll/api/Config.h>
#include <ll/api/service/Bedrock.h>
#include <mc/deps/core/mce/UUID.h>
#include <mc/deps/core/utility/BinaryStream.h>
#include <mc/network/NetworkConnection.h>
#include <mc/network/packet/AddActorBasePacket.h>
#include <mc/network/packet/BossEventPacket.h>
#include <mc/network/packet/Packet.h>
#include <mc/network/packet/PlayerListPacket.h>
#include <mc/network/packet/RemoveActorPacket.h>
#include <mc/network/packet/TextPacket.h>
#include <mc/server/LoopbackPacketSender.h>
#include <mc/world/ActorUniqueID.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/events/BossEventUpdateType.h>
#include <mc/world/level/Level.h>

void setPlayerBossbar(Player& player, bool enabled) {
    if (enabled) {
        // 创建bossbar需要的假实体
        if (!ll::service::getLevel()->fetchEntity(ActorUniqueID(config.bossbarID))) {
            // 摘抄至https://github.com/GroupMountain/GMLIB/blob/main/src/Server/BinaryStreamAPI.cc
            BinaryStream addActorBinaryStream;
            addActorBinaryStream.writeUnsignedVarInt(
                ((int)MinecraftPacketIds::AddActor & 0x3FF) | (((uchar)player.getClientSubId() & 3) << 10)
                | (((uchar)SubClientId::PrimaryClient & 3) << 12)
            );
            addActorBinaryStream.writeVarInt64(config.bossbarID);
            addActorBinaryStream.writeUnsignedVarInt64(config.bossbarID);
            addActorBinaryStream.writeString("player");
            addActorBinaryStream.writeFloat(player.getPosition().x);
            addActorBinaryStream.writeFloat(-66.0f);
            addActorBinaryStream.writeFloat(player.getPosition().z);
            for (int i = 0; i < 7; i++) addActorBinaryStream.writeFloat(0.0f);
            for (int i = 0; i < 5; i++) addActorBinaryStream.writeUnsignedVarInt(0);
            if (auto networksystem = ll::service::getNetworkSystem(); networksystem) {
                if (auto connection = networksystem->_getConnectionFromId(player.getNetworkIdentifier()); connection) {
                    if (auto peer = connection->mPeer; peer) {
                        peer->sendPacket(*addActorBinaryStream.mBuffer, NetworkPeer::Reliability(), Compressibility());
                    }
                }
            }
        }

        // 创建bossbar
        auto addBossbarPacket           = BossEventPacket();
        addBossbarPacket.mBossID        = ActorUniqueID(config.bossbarID);
        addBossbarPacket.mHealthPercent = 1.0f;
        addBossbarPacket.mEventType     = BossEventUpdateType::Add;
        addBossbarPacket.mColor         = BossBarColor::White;
        addBossbarPacket.mOverlay       = BossBarOverlay::Progress;
        addBossbarPacket.mName          = config.playerConfigs[player.getUuid()].vanishBossbarText;
        addBossbarPacket.sendTo(player);
    } else {
        // 移除bossbar
        auto removeBossbarPacket       = BossEventPacket();
        removeBossbarPacket.mEventType = BossEventUpdateType::Remove;
        removeBossbarPacket.mBossID    = ActorUniqueID(config.bossbarID);
        removeBossbarPacket.sendTo(player);

        // 移除bossbar创建的假实体
        if (!ll::service::getLevel()->fetchEntity(ActorUniqueID(config.bossbarID)))
            RemoveActorPacket(ActorUniqueID(config.bossbarID)).sendTo(player);
    }
}

void setPlayerVanishConfig(mce::UUID& uuid, Vanish::PlayerConfig playerConfig) {
    auto oldPlayerConfig = config.playerConfigs[uuid];
    auto level           = ll::service::getLevel();
    if (!level.has_value()) return;
    auto* player               = level->getPlayer(uuid);
    auto* packetSender         = static_cast<LoopbackPacketSender*>(level->getPacketSender());
    config.playerConfigs[uuid] = playerConfig;
    ll::config::saveConfig(config, Vanish::Entry::getInstance().getSelf().getConfigDir() / "config.json");
    if (player == nullptr) return;
    if (playerConfig.enabled) {
        // 移除玩家实体
        auto removePlayerPacket = RemoveActorPacket(ActorUniqueID(player->getOrCreateUniqueID()));
        level->forEachPlayer([&removePlayerPacket, &player](Player& player2) -> bool {
            if (*player != player2) removePlayerPacket.sendTo(player2);
            return true;
        });

        // 移除玩家列表
        auto removePlayerListPacket    = PlayerListPacket();
        removePlayerListPacket.mAction = PlayerListPacketType::Remove;
        removePlayerListPacket.emplace(PlayerListEntry(*player));
        packetSender->send(removePlayerListPacket);

        // 发送退出游戏提示
        if (oldPlayerConfig.enabled != playerConfig.enabled && playerConfig.vanishPromptExit) {
            // TextPacket::createTranslated("§e%multiplayer.player.left", {player->getRealName()}).sendToClients();
            auto leftMsgPacket = TextPacket::createTranslated("§e%multiplayer.player.left", {player->getRealName()});
            level->forEachPlayer([&leftMsgPacket](Player& player2) -> bool {
                player2.sendNetworkPacket(leftMsgPacket);
                return true;
            });
        }
    } else {
        // 添加玩家实体
        auto addPlayerPacket = player->tryCreateAddActorPacket();
        level->forEachPlayer([&addPlayerPacket, &player](Player& player2) -> bool {
            if (*player != player2) addPlayerPacket->sendTo(player2);
            return true;
        });

        // 恢复玩家列表
        auto addPlayerListPacket    = PlayerListPacket();
        addPlayerListPacket.mAction = PlayerListPacketType::Add;
        addPlayerListPacket.emplace(PlayerListEntry(*player));
        static_cast<LoopbackPacketSender*>(level->getPacketSender())->send(addPlayerListPacket);

        // 发送加入游戏提示
        if (oldPlayerConfig.enabled != playerConfig.enabled && playerConfig.appearPromptJoin) {
            TextPacket::createTranslated("§e%multiplayer.player.joined", {player->getRealName()}).sendToClients();
        }
    }
    setPlayerBossbar(*player, playerConfig.enabled ? playerConfig.vanishBossbar : false);
}