#pragma once

#include "Global.h"

#include <ll/api/memory/Hook.h>
#include <ll/api/service/Bedrock.h>
#include <mc/deps/raknet/RNS2_Windows_Linux_360.h>
#include <mc/deps/raknet/SystemAddress.h>
#include <mc/network/ServerNetworkHandler.h>
#include <mc/network/packet/AddActorBasePacket.h>
#include <mc/network/packet/PlayerListPacket.h>
#include <mc/network/packet/SetPlayerGameTypePacket.h>
#include <mc/network/packet/TextPacket.h>
#include <mc/server/LoopbackPacketSender.h>
#include <mc/server/ServerLevel.h>
#include <mc/server/ServerPlayer.h>
#include <mc/server/commands/CommandOrigin.h>
#include <mc/server/commands/CommandOutput.h>
#include <mc/server/commands/CommandSelectorBase.h>
#include <mc/world/ActorRuntimeID.h>
#include <mc/world/actor/ActorDamageByActorSource.h>
#include <mc/world/actor/ActorDamageByChildActorSource.h>
#include <mc/world/actor/ActorDamageSource.h>
#include <mc/world/actor/ActorDefinitionIdentifier.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/actor/player/PlayerListPacketType.h>
#include <mc/world/level/BlockSource.h>
#include <mc/world/level/Level.h>
#include <mc/world/level/block/Block.h>
#include <mc/world/level/block/actor/BarrelBlockActor.h>
#include <mc/world/level/block/actor/ChestBlockActor.h>
#include <mc/world/level/block/actor/RandomizableBlockActorContainerBase.h>
#include <mc/world/level/block/actor/ShulkerBoxBlockActor.h>
#include <mc/world/level/dimension/Dimension.h>
#include <mc/world/phys/AABB.h>
#include <optional>
#include <vector>

std::optional<mce::UUID> getUuid(std::string const& name) {
    if (auto level = ll::service::getLevel(); level.has_value()) {
        if (auto player = level->getPlayer(name); player) {
            return player->getUuid();
        }
    }
    return std::nullopt;
}
std::optional<mce::UUID> getUuid(ActorRuntimeID const& runtimeId) {
    if (auto level = ll::service::getLevel(); level.has_value()) {
        return level->getRuntimePlayer(runtimeId)->getUuid();
    }
    return std::nullopt;
}

// 玩家create包
LL_AUTO_TYPE_INSTANCE_HOOK(TryCreateActorPacketHook, ll::memory::HookPriority::Normal, Player, "?tryCreateAddActorPacket@Player@@UEAA?AV?$unique_ptr@VAddActorBasePacket@@U?$default_delete@VAddActorBasePacket@@@std@@@std@@XZ", std::unique_ptr<AddActorBasePacket>) {
    if (config.playerConfigs[getUuid()].enabled) {
        return nullptr;
    }
    return origin();
}

// 发送数据包给客户端
LL_AUTO_TYPE_INSTANCE_HOOK(
    SendBroadcastHook,
    ll::memory::HookPriority::Normal,
    LoopbackPacketSender,
    "?sendBroadcast@LoopbackPacketSender@@UEAAXAEBVNetworkIdentifier@@W4SubClientId@@AEBVPacket@@@Z",
    void,
    NetworkIdentifier const& exceptId,
    SubClientId              exceptSubid,
    Packet const&            packet
) {
    try {
        if (packet.getId() == MinecraftPacketIds::Text) {
            auto textPacket = static_cast<TextPacket const&>(packet);
            if (textPacket.mType == TextPacketType::Translate && textPacket.mParams.size() <= 2) {
                auto uuid = getUuid(textPacket.mParams[0]);
                if (uuid.has_value() && config.playerConfigs[uuid.value()].enabled) {
                    return;
                }
            }
        }
    } catch (...) {}
    origin(exceptId, exceptSubid, packet);
}
// 发送数据包给所有客户端
LL_AUTO_TYPE_INSTANCE_HOOK(
    SendBroadcastHook2,
    ll::memory::HookPriority::Normal,
    LoopbackPacketSender,
    "?sendBroadcast@LoopbackPacketSender@@UEAAXAEBVPacket@@@Z",
    void,
    Packet const& packet
) {
    try {
        if (packet.getId() == MinecraftPacketIds::Text) {
            auto textPacket = static_cast<TextPacket const&>(packet);
            if (textPacket.mType == TextPacketType::Translate && textPacket.mParams.size() <= 2) {
                auto uuid = getUuid(textPacket.mParams[0]);
                if (uuid.has_value() && config.playerConfigs[uuid.value()].enabled) {
                    return;
                }
            }
        }
    } catch (...) {}
    origin(packet);
}

// list变化
LL_AUTO_TYPE_INSTANCE_HOOK(
    PlayerListPacketHook,
    ll::memory::HookPriority::High,
    PlayerListPacket,
    &PlayerListPacket::emplace,
    void,
    PlayerListEntry&& entry
) {
    if (mAction == PlayerListPacketType::Add) {
        if (config.playerConfigs[entry.mUuid].enabled) {
            return;
        }
    }
    origin(std::move(entry));
}

// 移动音效
LL_AUTO_TYPE_INSTANCE_HOOK(
    PlayActorMovementSoundHook,
    ll::memory::HookPriority::Normal,
    Actor,
    &Actor::playMovementSound,
    void
) {
    if (isType(ActorType::Player)) {
        if (config.playerConfigs[((Player*)this)->getUuid()].enabled) {
            return;
        }
    }
    origin();
}

// 同步声音
LL_AUTO_TYPE_INSTANCE_HOOK(
    PlayActorSynchronizedSoundHook,
    ll::memory::HookPriority::Normal,
    Actor,
    &Actor::playSynchronizedSound,
    void,
    Puv::Legacy::LevelSoundEvent type,
    Vec3 const&                  pos,
    Block const&                 block,
    bool                         isGlobal
) {
    if (isType(ActorType::Player)) {
        if (config.playerConfigs[((Player*)this)->getUuid()].enabled) {
            return;
        }
    }
    origin(type, pos, block, isGlobal);
}

// 掉落到方块上声音
LL_AUTO_TYPE_INSTANCE_HOOK(
    OnActorFallOnHook,
    ll::memory::HookPriority::Normal,
    Block,
    &Block::onFallOn,
    void,
    BlockSource&    region,
    BlockPos const& pos,
    Actor&          entity,
    float           fallDistance
) {
    if (entity.isType(ActorType::Player)) {
        if (config.playerConfigs[((Player&)entity).getUuid()].enabled) {
            return;
        }
    }
    origin(region, pos, entity, fallDistance);
}

// 命令返回结果
LL_AUTO_TYPE_INSTANCE_HOOK(
    AddCommandMessageHook,
    ll::memory::HookPriority::Normal,
    CommandOutput,
    "?addMessage@CommandOutput@@AEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEBV?$vector@"
    "VCommandOutputParameter@@V?$allocator@VCommandOutputParameter@@@std@@@3@W4CommandOutputMessageType@@@Z",
    void,
    std::string&                        msgId,
    std::vector<CommandOutputParameter> params,
    CommandOutputMessageType            type
) {
    std::vector<std::string> playerNames;
    ll::service::getLevel()->forEachPlayer([&playerNames](Player& player) {
        if (!config.playerConfigs[player.getUuid()].enabled) {
            playerNames.push_back(player.getRealName());
        }
        return true;
    });
    if (msgId == "commands.players.list") {
        params[0].str = std::to_string(playerNames.size());
    }
    if (msgId == "commands.players.list.names") {
        std::ostringstream oss;
        bool               first = true;
        for (const auto& str : playerNames) {
            if (!first) oss << ", ";
            oss << str;
            first = false;
        }
        params[0].str = oss.str();
    }
    origin(msgId, params, type);
}

// 修复玩家切换游戏模式问题
LL_AUTO_TYPE_INSTANCE_HOOK(
    FixChangGameModeBugHook,
    ll::memory::HookPriority::Normal,
    Player,
    "?setPlayerGameType@Player@@UEAAXW4GameType@@@Z",
    void,
    GameType gameType
) {
    if (config.playerConfigs[getUuid()].enabled) {
        SetPlayerGameTypePacket packet;
        packet.mPlayerGameType = gameType;
        sendNetworkPacket(packet);
    }
    origin(gameType);
}

// 地图是否显示玩家
LL_AUTO_TYPE_INSTANCE_HOOK(
    CanBeSeenOnMapHook,
    ll::memory::HookPriority::Normal,
    Player,
    &Player::canBeSeenOnMap,
    bool
) {
    if (config.playerConfigs[getUuid()].enabled) {
        return false;
    }
    return origin();
}

// 玩家打开箱子
LL_AUTO_TYPE_INSTANCE_HOOK(
    StartOpenChestHook,
    ll::memory::HookPriority::Normal,
    ChestBlockActor,
    &ChestBlockActor::startOpen,
    void,
    Player& player
) {
    if (config.playerConfigs[player.getUuid()].enabled) {
        return;
    }
    origin(player);
}

// 玩家关闭箱子
LL_AUTO_TYPE_INSTANCE_HOOK(
    StopOpenChestHook,
    ll::memory::HookPriority::Normal,
    ChestBlockActor,
    &ChestBlockActor::stopOpen,
    void,
    Player& player
) {
    if (config.playerConfigs[player.getUuid()].enabled) {
        return;
    }
    origin(player);
}

// 玩家打开潜影盒
LL_AUTO_TYPE_INSTANCE_HOOK(
    StartOpenShulkerBoxHook,
    ll::memory::HookPriority::Normal,
    ShulkerBoxBlockActor,
    &ShulkerBoxBlockActor::startOpen,
    void,
    Player& player
) {
    if (config.playerConfigs[player.getUuid()].enabled) {
        return;
    }
    origin(player);
}

// 玩家关闭潜影盒
LL_AUTO_TYPE_INSTANCE_HOOK(
    StopOpenShulkerBoxHook,
    ll::memory::HookPriority::Normal,
    ShulkerBoxBlockActor,
    &ShulkerBoxBlockActor::stopOpen,
    void,
    Player& player
) {
    if (config.playerConfigs[player.getUuid()].enabled) {
        return;
    }
    origin(player);
}

// 玩家打开木桶
LL_AUTO_TYPE_INSTANCE_HOOK(
    StartOpenBarrelHook,
    ll::memory::HookPriority::Normal,
    BarrelBlockActor,
    &BarrelBlockActor::startOpen,
    void,
    Player& player
) {
    if (config.playerConfigs[player.getUuid()].enabled) {
        return;
    }
    origin(player);
}

// 玩家关闭木桶
LL_AUTO_TYPE_INSTANCE_HOOK(
    StopOpenBarrelHook,
    ll::memory::HookPriority::Normal,
    BarrelBlockActor,
    &BarrelBlockActor::stopOpen,
    void,
    Player& player
) {
    if (config.playerConfigs[player.getUuid()].enabled) {
        return;
    }
    origin(player);
}

// 玩家切换维度
LL_AUTO_TYPE_INSTANCE_HOOK(
    PlayerChangeDimensionHook,
    HookPriority::Normal,
    Level,
    &Level::requestPlayerChangeDimension,
    void,
    Player&                  player,
    ChangeDimensionRequest&& changeRequest
) {
    origin(player, std::move(changeRequest));
    auto& playerConfig = config.playerConfigs[player.getUuid()];
    if (playerConfig.enabled && playerConfig.vanishBossbar) {
        setPlayerBossbar(player, true);
    }
}

// 玩家能否睡觉
LL_AUTO_TYPE_INSTANCE_HOOK(PlayerCanSleepHook, ll::memory::HookPriority::Normal, Player, &Player::canSleep, bool) {
    if (config.playerConfigs[getUuid()].enabled) {
        return false;
    }
    return origin();
}

// 玩家捡起物品
LL_AUTO_TYPE_INSTANCE_HOOK(
    PlayerPickUpItemHook,
    HookPriority::Normal,
    Player,
    &Player::take,
    bool,
    Actor& itemActor,
    int    orgCount,
    int    favoredSlot
) {
    if (itemActor.hasCategory(ActorCategory::Item) && config.playerConfigs[getUuid()].enabled
        && config.playerConfigs[getUuid()].vanishNoTakeItem) {
        return false;
    }
    return origin(itemActor, orgCount, favoredSlot);
}

// 玩家丢出物品
LL_AUTO_TYPE_INSTANCE_HOOK(
    PlayerDropItemHook,
    HookPriority::Normal,
    Player,
    "?drop@Player@@UEAA_NAEBVItemStack@@_N@Z",
    bool,
    ItemStack const& item,
    bool             randomly
) {
    if (config.playerConfigs[getUuid()].enabled && config.playerConfigs[getUuid()].vanishNoDropItem) {
        return false;
    }
    return origin(item, randomly);
}

// 实体是否隐身
LL_AUTO_TYPE_INSTANCE_HOOK(
    PlayerIsInvisible,
    ll::memory::HookPriority::Normal,
    Actor,
    "?isInvisible@Actor@@UEBA_NXZ",
    bool
) {
    if (!isType(ActorType::Player)) return origin();
    return config.playerConfigs[((Player*)this)->getUuid()].enabled;
}

// 玩家进服
LL_AUTO_TYPE_INSTANCE_HOOK(
    PlayerJoinHook,
    HookPriority::Normal,
    ServerPlayer,
    &ServerPlayer::setLocalPlayerAsInitialized,
    void
) {
    origin();
    auto& playerConfig = config.playerConfigs[getUuid()];
    if (playerConfig.enabled && playerConfig.vanishBossbar) {
        setPlayerBossbar(*this, true);
    }
}

// 生物死亡信息
using DEATH_MESSAGE = std::pair<std::string, std::vector<std::string>>;
LL_AUTO_TYPE_INSTANCE_HOOK(
    ActorDamageByActorSourceHook,
    HookPriority::Normal,
    ActorDamageByActorSource,
    "?getDeathMessage@ActorDamageByActorSource@@UEBA?AU?$pair@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@"
    "std@@V?$vector@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@V?$allocator@V?$basic_string@DU?$"
    "char_traits@D@std@@V?$allocator@D@2@@std@@@2@@2@@std@@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@3@"
    "PEAVActor@@@Z",
    DEATH_MESSAGE,
    std::string deadName,
    Actor*      dead
) {
    if (auto level = ll::service::getLevel()) {
        auto* entity = level->fetchEntity(mEntityID);
        if (entity && entity->hasCategory(ActorCategory::Player)
            && config.playerConfigs[static_cast<Player*>(entity)->getUuid()].enabled) {
            mEntityID         = ActorUniqueID();
            mEntityType       = ActorType::None;
            mEntityCategories = ActorCategory::None;
            mEntityNameTag    = "";
        }
    }
    return origin(deadName, dead);
}
LL_AUTO_TYPE_INSTANCE_HOOK(
    ActorDamageByChildActorSourceHook,
    HookPriority::Normal,
    ActorDamageByChildActorSource,
    "?getDeathMessage@ActorDamageByChildActorSource@@UEBA?AU?$pair@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@"
    "D@2@@std@@V?$vector@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@V?$allocator@V?$basic_string@DU?"
    "$char_traits@D@std@@V?$allocator@D@2@@std@@@2@@2@@std@@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@3@"
    "PEAVActor@@@Z",
    DEATH_MESSAGE,
    std::string deadName,
    Actor*      dead
) {
    if (auto level = ll::service::getLevel()) {
        auto* entity = level->fetchEntity(mEntityID);
        if (entity && entity->hasCategory(ActorCategory::Player)
            && config.playerConfigs[static_cast<Player*>(entity)->getUuid()].enabled) {
            mDamagingActorId         = ActorUniqueID();
            mDamagingActorType       = ActorType::None;
            mDamagingActorCategories = ActorCategory::None;
            mDamagingActorNameTag    = "";
        }
    }
    return origin(deadName, dead);
}

struct RNS2_SendParameters {
    char*                 data;
    int                   length;
    RakNet::SystemAddress system_address;
    int                   ttl;
};
// Motd伪造 摘抄自 https://github.com/EndstoneMC/endstone/blob/main/src/endstone_runtime/bedrock/deps/raknet/raknet_socket2.cpp
LL_AUTO_TYPE_STATIC_HOOK(
    MotdCounterfeit,
    ll::memory::HookPriority::Normal,
    RakNet::RNS2_Windows_Linux_360,
    "?Send_Windows_Linux_360NoVDP@RNS2_Windows_Linux_360@RakNet@@KAHHPEAURNS2_SendParameters@2@PEBDI@Z",
    int,
    int                  rns2Socket,
    RNS2_SendParameters* sendParameters,
    char const*          file,
    uint                 line
) {
    try {
        if (sendParameters->data[0] == 28) {
            auto                 level     = ll::service::getLevel();
            constexpr static int head_size = sizeof(char) + sizeof(std::uint64_t) + sizeof(std::uint64_t) + 16;
            const char*          data      = sendParameters->data;
            std::size_t          strlen    = data[head_size] << 8 | data[head_size + 1];
            if (strlen != 0 && level.has_value()) {
                std::string ping_response{data + head_size + 2, strlen};
                char        buffer[64];
                sendParameters->system_address.ToString(false, buffer, '|');
                std::istringstream       iss(ping_response);
                std::string              tmp;
                std::vector<std::string> parts;
                while (std::getline(iss, tmp, ';')) {
                    parts.push_back(tmp);
                }

                int playerCount = 0;
                level->forEachPlayer([&playerCount](Player& player) -> bool {
                    if (!config.playerConfigs[player.getUuid()].enabled) playerCount++;
                    return true;
                });
                parts[4] = std::to_string(playerCount);

                std::string new_ping_response;
                for (size_t i = 0; i < parts.size(); ++i) new_ping_response += parts[i] + ';';
                std::vector<char> packet;
                packet.reserve(256);
                packet.insert(packet.end(), data, data + head_size);
                strlen = new_ping_response.length();
                packet.push_back(static_cast<char>((strlen >> 8) & 0xFF));
                packet.push_back(static_cast<char>(strlen & 0xFF));
                packet.insert(packet.end(), new_ping_response.begin(), new_ping_response.end());
                sendParameters->data   = packet.data();
                sendParameters->length = static_cast<int>(packet.size());
            }
        }
    } catch (...) {}
    return origin(rns2Socket, sendParameters, file, line);
}

// 实体设置目标(如仇恨)
LL_AUTO_TYPE_INSTANCE_HOOK(
    ActorIsValidTargetHook,
    ll::memory::HookPriority::Normal,
    Actor,
    "?setTarget@Actor@@UEAAXPEAV1@@Z",
    void,
    Actor* entity
) {
    if (entity && entity->isType(ActorType::Player)
        && config.playerConfigs[static_cast<Player*>(entity)->getUuid()].enabled) {
        return;
    }
    origin(entity);
}

// 目标选择器
LL_AUTO_TYPE_INSTANCE_HOOK(
    CommandSelectorHook,
    ll::memory::HookPriority::Normal,
    CommandSelectorBase,
    &CommandSelectorBase::newResults,
    std::shared_ptr<std::vector<Actor*>>,
    CommandOrigin const& commandOrigin
) {
    auto result = origin(commandOrigin);
    if (auto* entity = commandOrigin.getEntity()) {
        if (entity->isType(ActorType::Player) && config.playerConfigs[static_cast<Player*>(entity)->getUuid()].enabled)
            return result;
    }
    if (commandOrigin.getOriginType() != CommandOriginType::DedicatedServer && result) {
        for (auto& actor : *result) {
            if (actor->isType(ActorType::Player)
                && config.playerConfigs[static_cast<Player*>(actor)->getUuid()].enabled) {
                result->erase(std::remove(result->begin(), result->end(), actor), result->end());
            }
        }
    }
    return result;
}

// 获取最近的玩家
LL_AUTO_TYPE_INSTANCE_HOOK(
    FetchNearestPlayerHook,
    ll::memory::HookPriority::Normal,
    Dimension,
    &Dimension::fetchNearestPlayer,
    Player*,
    Vec3 const&                              pos,
    float                                    maxDist,
    bool                                     a3,
    std::function<bool(class Player const&)> func
) {
    return origin(pos, maxDist, a3, [&func](Player const& player) -> bool {
        return config.playerConfigs[player.getUuid()].enabled ? false : func(player);
    });
}

thread_local bool intercept = false;

// 玩家使用物品
LL_AUTO_TYPE_INSTANCE_HOOK(
    UseTimeDepletedHook,
    ll::memory::HookPriority::Normal,
    ItemStack,
    &ItemStack::useTimeDepleted,
    ItemUseMethod,
    Level*  level,
    Player* player
) {
    if (player && config.playerConfigs[player->getUuid()].enabled) intercept = true;
    auto result = origin(level, player);
    intercept   = false;
    return result;
}

// 广播声音
LL_AUTO_TYPE_INSTANCE_HOOK(
    broadcastSoundEventHook,
    ll::memory::HookPriority::Normal,
    Level,
    &Level::broadcastSoundEvent,
    void,
    BlockSource&                     region,
    Puv::Legacy::LevelSoundEvent     type,
    Vec3 const&                      pos,
    int                              data,
    ActorDefinitionIdentifier const& entityType,
    bool                             isBabyMob,
    bool                             isGlobal
) {
    if (intercept) return;
    origin(region, type, pos, data, entityType, isBabyMob, isGlobal);
}

// 广播声音
LL_AUTO_TYPE_INSTANCE_HOOK(
    broadcastSoundEventHook2,
    ll::memory::HookPriority::Normal,
    Level,
    &Level::broadcastSoundEvent,
    void,
    Dimension&                       dimension,
    Puv::Legacy::LevelSoundEvent     type,
    Vec3 const&                      pos,
    int                              data,
    ActorDefinitionIdentifier const& identifier,
    bool                             isBabyMob,
    bool                             isGlobal
) {
    if (intercept) return;
    origin(dimension, type, pos, data, identifier, isBabyMob, isGlobal);
}

// 广播声音
LL_AUTO_TYPE_INSTANCE_HOOK(
    broadcastSoundEventHook3,
    ll::memory::HookPriority::Normal,
    Level,
    &Level::broadcastSoundEvent,
    void,
    BlockSource&                     region,
    Puv::Legacy::LevelSoundEvent     type,
    Vec3 const&                      pos,
    Block const&                     block,
    ActorDefinitionIdentifier const& entityType,
    bool                             isBabyMob,
    bool                             isGlobal
) {
    if (intercept) return;
    origin(region, type, pos, block, entityType, isBabyMob, isGlobal);
}