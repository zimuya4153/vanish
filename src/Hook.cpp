#pragma once

#include "Global.h"

#include <ll/api/memory/Hook.h>
#include <ll/api/schedule/Scheduler.h>
#include <ll/api/service/Bedrock.h>
#include <mc/common/wrapper/InteractionResult.h>
#include <mc/deps/core/common/bedrock/LevelSoundManager.h>
#include <mc/deps/raknet/RNS2_Windows_Linux_360.h>
#include <mc/deps/raknet/SystemAddress.h>
#include <mc/entity/components/PushableComponent.h>
#include <mc/entity/utilities/ActorCollision.h>
#include <mc/network/ServerNetworkHandler.h>
#include <mc/network/packet/AddActorBasePacket.h>
#include <mc/network/packet/AnvilDamagePacket.h>
#include <mc/network/packet/InventorySlotPacket.h>
#include <mc/network/packet/PlayerActionPacket.h>
#include <mc/network/packet/PlayerListPacket.h>
#include <mc/network/packet/SetPlayerGameTypePacket.h>
#include <mc/network/packet/TextPacket.h>
#include <mc/server/LoopbackPacketSender.h>
#include <mc/server/ServerLevel.h>
#include <mc/server/ServerPlayer.h>
#include <mc/server/commands/CommandOrigin.h>
#include <mc/server/commands/CommandOutput.h>
#include <mc/server/commands/CommandSelectorBase.h>
#include <mc/server/commands/standard/ListCommand.h>
#include <mc/world/ActorRuntimeID.h>
#include <mc/world/actor/ActorDamageByActorSource.h>
#include <mc/world/actor/ActorDamageByChildActorSource.h>
#include <mc/world/actor/ActorDamageSource.h>
#include <mc/world/actor/ActorDefinitionIdentifier.h>
#include <mc/world/actor/item/ExperienceOrb.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/actor/player/PlayerDimensionTransferer.h>
#include <mc/world/actor/player/PlayerListPacketType.h>
#include <mc/world/actor/player/ServerPlayerBlockUseHandler.h>
#include <mc/world/containers/ContainerID.h>
#include <mc/world/gamemode/GameMode.h>
#include <mc/world/inventory/network/ItemStackRequestAction.h>
#include <mc/world/inventory/network/ItemStackRequestActionHandler.h>
#include <mc/world/inventory/transaction/ComplexInventoryTransaction.h>
#include <mc/world/level/BlockSource.h>
#include <mc/world/level/Level.h>
#include <mc/world/level/LevelEventManager.h>
#include <mc/world/level/block/Block.h>
#include <mc/world/level/block/TripWireBlock.h>
#include <mc/world/level/block/actor/BarrelBlockActor.h>
#include <mc/world/level/block/actor/ChestBlockActor.h>
#include <mc/world/level/block/actor/RandomizableBlockActorContainerBase.h>
#include <mc/world/level/block/actor/ShulkerBoxBlockActor.h>
#include <mc/world/level/dimension/Dimension.h>
#include <mc/world/phys/AABB.h>
#include <optional>
#include <vector>

thread_local bool               intercept = false;
ll::schedule::GameTickScheduler scheduler;

std::optional<mce::UUID> getUuid(std::string const& name) {
    if (auto level = ll::service::getLevel(); level.has_value()) {
        if (auto player = level->getPlayer(name); player) return player->getUuid();
    }
    return std::nullopt;
}
std::optional<mce::UUID> getUuid(ActorRuntimeID const& runtimeId) {
    if (auto level = ll::service::getLevel(); level.has_value()) return level->getRuntimePlayer(runtimeId)->getUuid();
    return std::nullopt;
}

// 玩家create包
LL_TYPE_INSTANCE_HOOK(TryCreateActorPacketHook, HookPriority::Normal, Player, "?tryCreateAddActorPacket@Player@@UEAA?AV?$unique_ptr@VAddActorBasePacket@@U?$default_delete@VAddActorBasePacket@@@std@@@std@@XZ", std::unique_ptr<AddActorBasePacket>) {
    return config.playerConfigs[getUuid()].enabled ? nullptr : origin();
}

// 发送数据包给客户端
LL_TYPE_INSTANCE_HOOK(
    SendBroadcastHook,
    HookPriority::Normal,
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
                if (uuid.has_value() && config.playerConfigs[uuid.value()].enabled) return;
            }
        }
    } catch (...) {}
    origin(exceptId, exceptSubid, packet);
}

// 发送数据包给所有客户端
LL_TYPE_INSTANCE_HOOK(
    SendBroadcastHook2,
    HookPriority::Normal,
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
                if (uuid.has_value() && config.playerConfigs[uuid.value()].enabled) return;
            }
        }
    } catch (...) {}
    origin(packet);
}

// list变化数据包安放玩家信息
LL_TYPE_INSTANCE_HOOK(
    PlayerListPacketEmplaceHook,
    ll::memory::HookPriority::High,
    PlayerListPacket,
    &PlayerListPacket::emplace,
    void,
    PlayerListEntry&& entry
) {
    if (mAction == PlayerListPacketType::Add && config.playerConfigs[entry.mUuid].enabled) return;
    origin(std::move(entry));
}

// 播放移动音效
LL_TYPE_INSTANCE_HOOK(PlayActorMovementSoundHook, HookPriority::Normal, Actor, &Actor::playMovementSound, void) {
    if (!isPlayer() || !config.playerConfigs[((Player*)this)->getUuid()].enabled) origin();
}

// 播放实体同步音效
LL_TYPE_INSTANCE_HOOK(
    PlayActorSynchronizedSoundHook,
    HookPriority::Normal,
    Actor,
    &Actor::playSynchronizedSound,
    void,
    Puv::Legacy::LevelSoundEvent type,
    Vec3 const&                  pos,
    Block const&                 block,
    bool                         isGlobal
) {
    if (!isPlayer() || !config.playerConfigs[((Player*)this)->getUuid()].enabled) origin(type, pos, block, isGlobal);
}

// 实体掉落到方块上
LL_TYPE_INSTANCE_HOOK(
    ActorFallOnHook,
    HookPriority::Normal,
    Block,
    &Block::onFallOn,
    void,
    BlockSource&    region,
    BlockPos const& pos,
    Actor&          entity,
    float           fallDistance
) {
    if (entity.isPlayer() && config.playerConfigs[((Player&)entity).getUuid()].enabled) intercept = true;
    origin(region, pos, entity, fallDistance);
    intercept = false;
}

// 修改list命令返回结果
LL_TYPE_INSTANCE_HOOK(
    ListCmdExecuteHook,
    HookPriority::Low,
    ListCommand,
    "?execute@ListCommand@@UEBAXAEBVCommandOrigin@@AEAVCommandOutput@@@Z",
    void,
    CommandOrigin const&,
    CommandOutput& output
) {
    std::string playerNameList = "";
    auto        playerCount    = 0;
    bool        first          = true;
    ll::service::getLevel()->forEachPlayer([&playerNameList, &playerCount, &first](Player& player) -> bool {
        if (!config.playerConfigs[player.getUuid()].enabled) {
            if (!first) playerNameList += ", ";
            playerNameList += player.getRealName();
            fmt::join(playerNameList, ", ");
            playerCount++;
        }
        return true;
    });
    output.success(
        "commands.players.list",
        {CommandOutputParameter(playerCount),
         // 摘抄自: https://github.com/GroupMountain/GMLIB/blob/main/src/Server/LevelAPI.cc#L839-L842
         CommandOutputParameter(ll::memory::dAccess<int>(ll::service::getServerNetworkHandler().as_ptr(), 200 * 4))}
    );
    output.success("commands.players.list.names", {CommandOutputParameter(playerNameList)});
}

// 修复玩家切换游戏模式问题
LL_TYPE_INSTANCE_HOOK(
    SetPlayerGameTypeHook,
    HookPriority::Low,
    Player,
    "?setPlayerGameType@Player@@UEAAXW4GameType@@@Z",
    void,
    GameType gameType
) {
    origin(gameType);
    if (config.playerConfigs[getUuid()].enabled) {
        SetPlayerGameTypePacket packet;
        packet.mPlayerGameType = gameType;
        sendNetworkPacket(packet);
    }
}

// 地图是否显示玩家
LL_TYPE_INSTANCE_HOOK(CanBeSeenOnMapHook, HookPriority::Normal, Player, &Player::canBeSeenOnMap, bool) {
    return config.playerConfigs[getUuid()].enabled ? false : origin();
}

// 玩家打开箱子
LL_TYPE_INSTANCE_HOOK(
    StartOpenChestHook,
    HookPriority::Normal,
    ChestBlockActor,
    &ChestBlockActor::startOpen,
    void,
    Player& player
) {
    if (config.playerConfigs[player.getUuid()].enabled) {
        static_cast<RandomizableBlockActorFillingContainer*>(this)->startOpen(player);
        if (*((uint64_t*)this + 54) && (*((uint64_t*)this + 396) & 1) != 0)
            static_cast<RandomizableBlockActorFillingContainer*>(this + 240)->startOpen(player);
        return;
    }
    origin(player);
}

// 玩家关闭箱子
LL_TYPE_INSTANCE_HOOK(
    StopOpenChestHook,
    HookPriority::Normal,
    ChestBlockActor,
    &ChestBlockActor::stopOpen,
    void,
    Player& player
) {
    if (!config.playerConfigs[player.getUuid()].enabled) origin(player);
}

// 玩家打开潜影盒
LL_TYPE_INSTANCE_HOOK(
    StartOpenShulkerBoxHook,
    HookPriority::Normal,
    ShulkerBoxBlockActor,
    &ShulkerBoxBlockActor::startOpen,
    void,
    Player& player
) {
    if (!config.playerConfigs[player.getUuid()].enabled) origin(player);
}

// 玩家关闭潜影盒
LL_TYPE_INSTANCE_HOOK(
    StopOpenShulkerBoxHook,
    HookPriority::Normal,
    ShulkerBoxBlockActor,
    &ShulkerBoxBlockActor::stopOpen,
    void,
    Player& player
) {
    if (!config.playerConfigs[player.getUuid()].enabled) origin(player);
}

// 玩家打开木桶
LL_TYPE_INSTANCE_HOOK(
    StartOpenBarrelHook,
    HookPriority::Normal,
    BarrelBlockActor,
    &BarrelBlockActor::startOpen,
    void,
    Player& player
) {
    if (!config.playerConfigs[player.getUuid()].enabled) origin(player);
}

// 玩家关闭木桶
LL_TYPE_INSTANCE_HOOK(
    StopOpenBarrelHook,
    HookPriority::Normal,
    BarrelBlockActor,
    &BarrelBlockActor::stopOpen,
    void,
    Player& player
) {
    if (!config.playerConfigs[player.getUuid()].enabled) origin(player);
}

// 玩家切换维度
LL_TYPE_INSTANCE_HOOK(
    PlayerChangeDimensionHook,
    HookPriority::Normal,
    PlayerDimensionTransferer,
    "?playerPrepareRegion@PlayerDimensionTransferer@@UEAAXAEAVPlayer@@AEBVChangeDimensionRequest@@AEBVDimension@@@Z",
    void,
    Player&                       player,
    const ChangeDimensionRequest& toDimension,
    const Dimension&              dimension
) {
    origin(player, toDimension, dimension);
    scheduler.add<ll::schedule::DelayTask>(ll::chrono::ticks(20 * 5), [&player]() -> void {
        auto& playerConfig = config.playerConfigs[player.getUuid()];
        if (playerConfig.enabled && playerConfig.vanishBossbar) setPlayerBossbar(player, true);
    });
}

// 玩家能否睡觉
LL_TYPE_INSTANCE_HOOK(PlayerCanSleepHook, HookPriority::Normal, Player, &Player::canSleep, bool) {
    return config.playerConfigs[getUuid()].enabled ? false : origin();
}

// 玩家捡起物品
LL_TYPE_INSTANCE_HOOK(
    PlayerTakeItemHook,
    HookPriority::Normal,
    Player,
    &Player::take,
    bool,
    Actor& itemActor,
    int    orgCount,
    int    favoredSlot
) {
    return itemActor.hasCategory(ActorCategory::Item) && config.playerConfigs[getUuid()].enabled
                && config.playerConfigs[getUuid()].vanishNoTakeItem
             ? false
             : origin(itemActor, orgCount, favoredSlot);
}

// 处理背包操作
LL_TYPE_INSTANCE_HOOK(
    HandleComplexInventoryTransactionHook,
    HookPriority::Normal,
    ComplexInventoryTransaction,
    "?handle@ComplexInventoryTransaction@@UEBA?AW4InventoryTransactionError@@AEAVPlayer@@_N@Z",
    InventoryTransactionError,
    Player& player,
    bool    isSenderAuthority
) {
    if (type == ComplexInventoryTransaction::Type::NormalTransaction && config.playerConfigs[player.getUuid()].enabled
        && config.playerConfigs[player.getUuid()].vanishNoDropItem) {
        auto& actions =
            data.getActions(InventorySource(InventorySourceType::ContainerInventory, ContainerID::Inventory));
        InventorySlotPacket(
            ContainerID::Inventory,
            actions.data()->mSlot,
            player.getInventory().getItem(actions[0].mSlot)
        )
            .sendTo(player);
        return InventoryTransactionError::NoError;
    }
    return origin(player, isSenderAuthority);
}

// 处理玩家物品操作请求
LL_TYPE_INSTANCE_HOOK(
    HandleRequestActionHook,
    HookPriority::Normal,
    ItemStackRequestActionHandler,
    "?handleRequestAction@ItemStackRequestActionHandler@@QEAA?AW4ItemStackNetResult@@AEBVItemStackRequestAction@@@Z",
    ItemStackNetResult,
    ItemStackRequestAction& requestAction
) {
    if (requestAction.mActionType == ItemStackRequestActionType::Drop && config.playerConfigs[mPlayer.getUuid()].enabled
        && config.playerConfigs[mPlayer.getUuid()].vanishNoDropItem)
        return ItemStackNetResult::Error;
    return origin(requestAction);
}

// 实体是否隐身
LL_TYPE_INSTANCE_HOOK(PlayerIsInvisible, HookPriority::Normal, Actor, "?isInvisible@Actor@@UEBA_NXZ", bool) {
    return isPlayer() ? config.playerConfigs[((Player*)this)->getUuid()].enabled : origin();
}

// 设置实体隐身
LL_TYPE_INSTANCE_HOOK(
    SetPlayerInvisibleHook,
    HookPriority::Normal,
    Actor,
    "?setInvisible@Actor@@QEAAX_N@Z",
    void,
    bool value
) {
    if (!isPlayer()) return origin(value);
    playSynchronizedSound(
        static_cast<Puv::Legacy::LevelSoundEvent>(static_cast<int>(value) ^ 1 + 430),
        getAttachPos(ActorLocation::Body),
        1,
        false
    );
    setStatusFlag(ActorFlags::CanShowName, value ^ 1);
    setStatusFlag(ActorFlags::Invisible, value);
}

// 玩家进服
LL_TYPE_INSTANCE_HOOK(
    PlayerJoinHook,
    HookPriority::Normal,
    ServerPlayer,
    &ServerPlayer::setLocalPlayerAsInitialized,
    void
) {
    origin();
    auto& playerConfig = config.playerConfigs[getUuid()];
    if (playerConfig.enabled && playerConfig.vanishBossbar) setPlayerBossbar(*this, true);
}

using DEATH_MESSAGE = std::pair<std::string, std::vector<std::string>>;
// 生物死亡信息
LL_TYPE_INSTANCE_HOOK(
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
    if (auto level = ll::service::getLevel(); level.has_value()) {
        auto* entity = level->fetchEntity(mEntityID);
        if (entity && entity->isPlayer() && config.playerConfigs[static_cast<Player*>(entity)->getUuid()].enabled) {
            mEntityID         = ActorUniqueID();
            mEntityType       = ActorType::None;
            mEntityCategories = ActorCategory::None;
            mEntityNameTag    = "";
        }
    }
    return origin(deadName, dead);
}

// 生物死亡信息
LL_TYPE_INSTANCE_HOOK(
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
    if (auto level = ll::service::getLevel(); level.has_value()) {
        auto* entity = level->fetchEntity(mEntityID);
        if (entity && entity->isPlayer() && config.playerConfigs[static_cast<Player*>(entity)->getUuid()].enabled) {
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
enum MessageIdentifiers : uchar { UnconnectedPong = 28 };
// 发送通信数据
// 伪造部分摘抄自
// https://github.com/EndstoneMC/endstone/blob/main/src/endstone_runtime/bedrock/deps/raknet/raknet_socket2.cpp
LL_TYPE_STATIC_HOOK(
    SendWindowsLinux360NoVDPHook,
    HookPriority::Normal,
    RakNet::RNS2_Windows_Linux_360,
    "?Send_Windows_Linux_360NoVDP@RNS2_Windows_Linux_360@RakNet@@KAHHPEAURNS2_SendParameters@2@PEBDI@Z",
    int,
    int                  rns2Socket,
    RNS2_SendParameters* sendParameters,
    char const*          file,
    uint                 line
) {
    try {
        if (sendParameters->data[0] == MessageIdentifiers::UnconnectedPong) {
            auto                 level     = ll::service::getLevel();
            constexpr static int head_size = sizeof(char) + sizeof(std::uint64_t) + sizeof(std::uint64_t) + 16;
            const char*          data      = sendParameters->data;
            std::size_t          strlen    = data[head_size] << 8 | data[head_size + 1];
            if (strlen != 0 && level.has_value()) {
                std::string              ping_response{data + head_size + 2, strlen};
                std::istringstream       iss(ping_response);
                std::string              tmp;
                std::vector<std::string> parts;
                while (std::getline(iss, tmp, ';')) parts.push_back(tmp);

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
                return origin(rns2Socket, sendParameters, file, line);
            }
        }
    } catch (...) {}
    return origin(rns2Socket, sendParameters, file, line);
}

// 实体设置目标(如仇恨)
LL_TYPE_INSTANCE_HOOK(
    ActorIsValidTargetHook,
    HookPriority::Normal,
    Actor,
    "?setTarget@Actor@@UEAAXPEAV1@@Z",
    void,
    Actor* entity
) {
    if (entity == nullptr || !entity->isPlayer()
        || !config.playerConfigs[static_cast<Player*>(entity)->getUuid()].enabled)
        origin(entity);
}

// 目标选择器
LL_TYPE_INSTANCE_HOOK(
    CommandSelectorGetResultHook,
    HookPriority::Normal,
    CommandSelectorBase,
    &CommandSelectorBase::newResults,
    std::shared_ptr<std::vector<Actor*>>,
    CommandOrigin const& commandOrigin
) {
    auto result = origin(commandOrigin);
    if (auto* entity = commandOrigin.getEntity();
        entity && entity->isPlayer() && config.playerConfigs[static_cast<Player*>(entity)->getUuid()].enabled)
        return result;
    if (commandOrigin.getOriginType() != CommandOriginType::DedicatedServer && result) {
        for (auto& actor : *result) {
            if (actor->isPlayer() && config.playerConfigs[static_cast<Player*>(actor)->getUuid()].enabled)
                result->erase(std::remove(result->begin(), result->end(), actor), result->end());
        }
    }
    return result;
}

// 获取最近的玩家
LL_TYPE_INSTANCE_HOOK(
    FetchNearestPlayerHook,
    HookPriority::Normal,
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

// 玩家使用物品
LL_TYPE_INSTANCE_HOOK(
    UseTimeDepletedHook,
    HookPriority::Normal,
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
LL_TYPE_INSTANCE_HOOK(
    broadcastSoundEventHook,
    HookPriority::Normal,
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
    if (!intercept) origin(region, type, pos, data, entityType, isBabyMob, isGlobal);
}

// 广播声音
LL_TYPE_INSTANCE_HOOK(
    broadcastSoundEventHook2,
    HookPriority::Normal,
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
    if (!intercept) origin(dimension, type, pos, data, identifier, isBabyMob, isGlobal);
}

// 广播声音
LL_TYPE_INSTANCE_HOOK(
    broadcastSoundEventHook3,
    HookPriority::Normal,
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
    if (!intercept) origin(region, type, pos, block, entityType, isBabyMob, isGlobal);
}

// 局部广播事件
LL_TYPE_INSTANCE_HOOK(
    BroadcastLocalEventHook,
    HookPriority::Normal,
    LevelEventManager,
    &LevelEventManager::broadcastLocalEvent,
    void,
    IDimension& a1,
    LevelEvent  type,
    Vec3 const& pos,
    int         a4
) {
    if (!intercept) origin(a1, type, pos, a4);
}

// 全局广播事件
LL_TYPE_INSTANCE_HOOK(
    BroadcastLevelEventHook,
    HookPriority::Normal,
    LevelEventManager,
    &LevelEventManager::broadcastLevelEvent,
    void,
    LevelEvent                           type,
    Vec3 const&                          pos,
    int                                  a3,
    UserEntityIdentifierComponent const* a4
) {
    if (!intercept) origin(type, pos, a3, a4);
}

// 播放声音
LL_TYPE_INSTANCE_HOOK(
    PlaySoundHook,
    HookPriority::Normal,
    LevelSoundManager,
    &LevelSoundManager::playSound,
    void,
    Puv::Legacy::LevelSoundEvent     type,
    Vec3 const&                      pos,
    int                              data,
    ActorDefinitionIdentifier const& entityType,
    bool                             isBabyMob,
    bool                             isGlobal
) {
    if (!intercept) origin(type, pos, data, entityType, isBabyMob, isGlobal);
}

// 处理玩家动作
LL_TYPE_INSTANCE_HOOK(
    HandlerPlayerActionPacketHook,
    HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::handle,
    void,
    NetworkIdentifier const&  source,
    PlayerActionPacket const& packet
) {
    if (auto player = getServerPlayer(source, packet.mClientSubId);
        player.has_value() && config.playerConfigs[player->getUuid()].enabled)
        intercept = true;
    origin(source, packet);
    intercept = false;
}

// 物品对着方块使用
LL_TYPE_INSTANCE_HOOK(
    ItemUseOnHook,
    HookPriority::Normal,
    ItemStack,
    &ItemStack::useOn,
    InteractionResult,
    Actor&      entity,
    int         x,
    int         y,
    int         z,
    uchar       face,
    Vec3 const& clickPos
) {
    if (entity.isPlayer() && config.playerConfigs[static_cast<Player&>(entity).getUuid()].enabled) intercept = true;
    auto result = origin(entity, x, y, z, face, clickPos);
    intercept   = false;
    return result;
}

// 玩家破坏方块
LL_STATIC_HOOK(
    PlayerDestroyBlockHook,
    HookPriority::Normal,
    &ServerPlayerBlockUseHandler::onStartDestroyBlock,
    void,
    ServerPlayer&   player,
    BlockPos const& pos,
    int             face
) {
    if (config.playerConfigs[player.getUuid()].enabled) intercept = true;
    origin(player, pos, face);
    intercept = false;
}

// 破坏方块
LL_TYPE_INSTANCE_HOOK(
    GameModeDestroyBlockHook,
    HookPriority::Normal,
    GameMode,
    "?destroyBlock@GameMode@@UEAA_NAEBVBlockPos@@E@Z",
    bool,
    BlockPos const& pos,
    uchar           face
) {
    if (config.playerConfigs[getPlayer().getUuid()].enabled) intercept = true;
    auto result = origin(pos, face);
    intercept   = false;
    return result;
}

// 玩家是否拥有能力能力
LL_TYPE_INSTANCE_HOOK(
    PlayerCanUseAbilityHook,
    HookPriority::Normal,
    Player,
    &Player::canUseAbility,
    bool,
    AbilitiesIndex abilityIndex
) {
    return abilityIndex == AbilitiesIndex::DoorsAndSwitches && config.playerConfigs[getUuid()].enabled
                && config.playerConfigs[getUuid()].vanishNoRedstone
             ? false
             : origin(abilityIndex);
}

// 绊线钩是否可以触发
LL_TYPE_INSTANCE_HOOK(
    IsEntityInsideTriggerableHook,
    HookPriority::Normal,
    TripWireBlock,
    &TripWireBlock::_isEntityInsideTriggerable,
    bool,
    BlockSource const& region,
    BlockPos const&    block,
    Actor&             entity
) {
    if (entity.isPlayer()) {
        auto& player = static_cast<Player&>(entity);
        if (config.playerConfigs[player.getUuid()].enabled && config.playerConfigs[player.getUuid()].vanishNoRedstone)
            return false;
    }
    return origin(region, block, entity);
}

// 物品使用
LL_TYPE_INSTANCE_HOOK(ItemUseHook, HookPriority::Normal, ItemStack, &ItemStack::use, ItemStack&, Player& player) {
    if (config.playerConfigs[player.getUuid()].enabled) intercept = true;
    auto& result = origin(player);
    intercept    = false;
    return result;
}

// 方块接受交互
LL_TYPE_INSTANCE_HOOK(
    PlayerInteractBlockHook,
    HookPriority::Normal,
    GameMode,
    "?useItemOn@GameMode@@UEAA?AVInteractionResult@@AEAVItemStack@@AEBVBlockPos@@EAEBVVec3@@PEBVBlock@@@Z",
    InteractionResult,
    ItemStack&      item,
    BlockPos const& blockPos,
    uchar           face,
    Vec3 const&     clickPos,
    Block const*    block
) {
    if (config.playerConfigs[getPlayer().getUuid()].enabled
        && config.playerConfigs[getPlayer().getUuid()].vanishNoInteractSound)
        intercept = true;
    auto result = origin(item, blockPos, face, clickPos, block);
    intercept   = false;
    return result;
}

// 处理铁砧使用数据包
LL_TYPE_INSTANCE_HOOK(
    HandleAnvilDamagePacketHook,
    HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::handle,
    void,
    NetworkIdentifier const& source,
    AnvilDamagePacket const& packet
) {
    if (auto player = getServerPlayer(source, packet.mClientSubId)) {
        if (config.playerConfigs[player->getUuid()].enabled) intercept = true;
    }
    origin(source, packet);
    intercept = false;
}

// 触碰经验球
LL_TYPE_INSTANCE_HOOK(
    PlayerTouchExperienceOrbHook,
    HookPriority::Normal,
    ExperienceOrb,
    "?playerTouch@ExperienceOrb@@UEAAXAEAVPlayer@@@Z",
    void,
    Player& player
) {
    if (!config.playerConfigs[player.getUuid()].enabled || !config.playerConfigs[player.getUuid()].vanishNoTouchEntity)
        origin(player);
}

// 实体推动
LL_TYPE_INSTANCE_HOOK(
    PushableComponentPushHook,
    HookPriority::Normal,
    PushableComponent,
    &PushableComponent::push,
    void,
    Actor&      owner,
    Vec3 const& vec
) {
    if (owner.isPlayer()) {
        auto& player = static_cast<Player&>(owner);
        if (config.playerConfigs[player.getUuid()].enabled
            && config.playerConfigs[player.getUuid()].vanishNoTouchEntity)
            return;
    }
    origin(owner, vec);
}

// 实体推动
LL_TYPE_INSTANCE_HOOK(
    PushableComponentPushHook2,
    HookPriority::Normal,
    PushableComponent,
    &PushableComponent::push,
    void,
    Actor& owner,
    Actor& other,
    bool   pushSelfOnly
) {
    if (owner.isPlayer()) {
        auto& player = static_cast<Player&>(owner);
        if (config.playerConfigs[player.getUuid()].enabled
            && config.playerConfigs[player.getUuid()].vanishNoTouchEntity)
            return;
    }
    if (other.isPlayer()) {
        auto& player = static_cast<Player&>(other);
        if (config.playerConfigs[player.getUuid()].enabled
            && config.playerConfigs[player.getUuid()].vanishNoTouchEntity)
            return;
    }
    origin(owner, other, pushSelfOnly);
}

LL_STATIC_HOOK(
    IsPickableHook,
    HookPriority::Normal,
    "?isPickable@ActorCollision@@YA_NAEBVEntityContext@@@Z",
    bool,
    EntityContext const& context
) {
    if (auto actor = Actor::tryGetFromEntity(context, true)) {
        if (actor->isPlayer()) {
            auto player = static_cast<const Player*>(actor);
            if (config.playerConfigs[player->getUuid()].enabled) return false;
        }
    }
    return origin(context);
}

struct Impl {
    ll::memory::HookRegistrar<
        TryCreateActorPacketHook,
        SendBroadcastHook,
        SendBroadcastHook2,
        PlayerListPacketEmplaceHook,
        PlayActorMovementSoundHook,
        PlayActorSynchronizedSoundHook,
        ActorFallOnHook,
        ListCmdExecuteHook,
        SetPlayerGameTypeHook,
        CanBeSeenOnMapHook,
        StartOpenChestHook,
        StopOpenChestHook,
        StartOpenShulkerBoxHook,
        StopOpenShulkerBoxHook,
        StartOpenBarrelHook,
        StopOpenBarrelHook,
        PlayerChangeDimensionHook,
        PlayerCanSleepHook,
        PlayerTakeItemHook,
        HandleComplexInventoryTransactionHook,
        PlayerIsInvisible,
        SetPlayerInvisibleHook,
        PlayerJoinHook,
        ActorDamageByActorSourceHook,
        ActorDamageByChildActorSourceHook,
        SendWindowsLinux360NoVDPHook,
        ActorIsValidTargetHook,
        CommandSelectorGetResultHook,
        FetchNearestPlayerHook,
        UseTimeDepletedHook,
        broadcastSoundEventHook,
        broadcastSoundEventHook2,
        broadcastSoundEventHook3,
        HandlerPlayerActionPacketHook,
        ItemUseOnHook,
        BroadcastLocalEventHook,
        BroadcastLevelEventHook,
        PlayerDestroyBlockHook,
        HandleRequestActionHook,
        PlayerCanUseAbilityHook,
        IsEntityInsideTriggerableHook,
        GameModeDestroyBlockHook,
        ItemUseHook,
        PlayerInteractBlockHook,
        HandleAnvilDamagePacketHook,
        PlaySoundHook,
        PlayerTouchExperienceOrbHook,
        PushableComponentPushHook,
        PushableComponentPushHook2,
        IsPickableHook>
        hook;
};

std::unique_ptr<Impl> impl;

void HookCall(bool enable) {
    if (enable) {
        if (!impl) impl = std::make_unique<Impl>();
    } else impl.reset();
}