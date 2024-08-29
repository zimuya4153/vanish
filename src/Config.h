#pragma once

#include <mc/deps/core/mce/UUID.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace Vanish {
struct PlayerConfig {
    bool        enabled           = false;           // 是否启用隐身
    bool        appearPromptJoin  = true;            // 解除隐身后提示加入游戏
    bool        vanishPromptExit  = true;            // 隐身后提示退出游戏
    bool        vanishBossbar     = true;            // 隐身后显示Boss栏
    std::string vanishBossbarText = "§6爷隐身了OvO"; // 隐身Boss栏文本
    bool        vanishNoTakeItem  = false;           // 隐身后禁止拿取物品
    bool        vanishNoDropItem  = false;           // 隐身后禁止丢出物品
    bool        vanishNoRedstone  = false;           // 隐身后禁止触发红石机关
};
struct Config {
    int                                         version     = 2;        // 配置文件版本
    std::string                                 command     = "vanish"; // 命令名
    std::string                                 alias       = "v";      // 命令别名
    int64_t                                     bossbarID   = 92732814; // Boss栏ID
    bool                                        enabledPAPI = true;     // 注册PAPI变量
    bool                                        permMode    = false;    // 权限模式
    std::vector<mce::UUID>                      permPlayers;            // 权限玩家
    std::unordered_map<mce::UUID, PlayerConfig> playerConfigs;          // 玩家配置
};
} // namespace Vanish