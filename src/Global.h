#pragma once

#include "Config.h"

#include <ll/api/Logger.h>
#include <ll/api/data/Version.h>

#define VERSION ll::data::Version(2, 0, 4)

extern ll::Logger     logger;
extern Vanish::Config config;

extern void registerCommands();
extern void setPlayerVanishConfig(mce::UUID& uuid, Vanish::PlayerConfig playerConfig);
extern void setPlayerBossbar(class Player& player, bool enabled);
extern void PapiCall(bool enable);
extern void HookCall(bool enable);