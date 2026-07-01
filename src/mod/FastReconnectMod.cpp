#include "mod/FastReconnectMod.h"

#include "mod/ClientInstanceHooks.h"
#include "mod/ReconnectManager.h"

#include "ll/api/Config.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/client/ClientJoinLevelEvent.h"
#include "ll/api/event/client/ClientStartJoinLevelEvent.h"
#include "ll/api/mod/RegisterHelper.h"

namespace fast_reconnect {

FastReconnectMod& FastReconnectMod::getInstance() {
    static FastReconnectMod instance;
    return instance;
}

bool FastReconnectMod::load() {
    auto const configPath = getSelf().getConfigDir() / u8"config.json";
    if (!ll::config::loadConfig(mConfig, configPath)) {
        ll::config::saveConfig(mConfig, configPath);
    }
    ReconnectManager::getInstance().setConfig(mConfig);
    return true;
}

bool FastReconnectMod::enable() {
    registerHooks();

    auto& bus = ll::event::EventBus::getInstance();

    mStartJoinListener = bus.emplaceListener<ll::event::ClientStartJoinLevelEvent>(
        [](ll::event::ClientStartJoinLevelEvent& event) {
            ReconnectManager::getInstance().onStartJoin(event.isJoiningLocalServer(), event.serverName());
        }
    );
    mJoinLevelListener = bus.emplaceListener<ll::event::ClientJoinLevelEvent>(
        [](ll::event::ClientJoinLevelEvent&) { ReconnectManager::getInstance().onJoinedLevel(); }
    );

    getSelf().getLogger().info("FastReconnect enabled");
    return true;
}

bool FastReconnectMod::disable() {
    auto& bus = ll::event::EventBus::getInstance();
    if (mStartJoinListener) {
        bus.removeListener(mStartJoinListener);
        mStartJoinListener.reset();
    }
    if (mJoinLevelListener) {
        bus.removeListener(mJoinLevelListener);
        mJoinLevelListener.reset();
    }

    unregisterHooks();
    ReconnectManager::getInstance().reset();
    return true;
}

} // namespace fast_reconnect

LL_REGISTER_MOD(fast_reconnect::FastReconnectMod, fast_reconnect::FastReconnectMod::getInstance());
