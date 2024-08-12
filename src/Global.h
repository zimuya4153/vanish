#pragma once

#include "Config.h"

#include <ll/api/Logger.h>

extern Vanish::Config config;
extern ll::Logger logger;

extern void registerCommands();
extern void setPlayerVanishConfig(mce::UUID& uuid, Vanish::PlayerConfig playerConfig);
extern void setPlayerBossbar(class Player& player, bool enabled);
extern void registerPAPI();
extern void unregisterAllPAPI();