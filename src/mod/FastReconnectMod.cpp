#include "mod/FastReconnectMod.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "mod/ClientInstanceHooks.h"
#include "mod/ReconnectManager.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/client/ClientJoinLevelEvent.h"
#include "ll/api/event/client/ClientStartJoinLevelEvent.h"
#include "ll/api/mod/RegisterHelper.h"

#include "nlohmann/json.hpp"

namespace fast_reconnect {

FastReconnectMod& FastReconnectMod::getInstance() {
    static FastReconnectMod instance;
    return instance;
}

bool FastReconnectMod::load() {
    namespace fs          = std::filesystem;
    auto const configPath = getSelf().getConfigDir() / u8"config.json";

    std::error_code ec;
    if (fs::exists(configPath, ec)) {
        std::ifstream in(configPath);
        if (in) {
            std::stringstream buffer;
            buffer << in.rdbuf();
            auto json = nlohmann::json::parse(buffer.str(), nullptr, false, true);
            if (!json.is_discarded() && json.is_object()) {
                mConfig.enabled         = json.value("enabled", mConfig.enabled);
                mConfig.delaySeconds    = json.value("delaySeconds", mConfig.delaySeconds);
                mConfig.maxAttempts     = json.value("maxAttempts", mConfig.maxAttempts);
                mConfig.reconnectOnKick = json.value("reconnectOnKick", mConfig.reconnectOnKick);
            } else {
                getSelf().getLogger().warn("Config is not valid JSON, using defaults");
            }
        }
    }

    nlohmann::ordered_json out{
        {"version",         mConfig.version        },
        {"enabled",         mConfig.enabled        },
        {"delaySeconds",    mConfig.delaySeconds   },
        {"maxAttempts",     mConfig.maxAttempts    },
        {"reconnectOnKick", mConfig.reconnectOnKick}
    };
    fs::create_directories(configPath.parent_path(), ec);
    std::ofstream outFile(configPath, std::ios::trunc);
    if (outFile) {
        outFile << out.dump(4);
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
    for (auto* listener : {&mStartJoinListener, &mJoinLevelListener}) {
        if (*listener) {
            bus.removeListener(*listener);
            listener->reset();
        }
    }

    unregisterHooks();
    ReconnectManager::getInstance().reset();
    return true;
}

} // namespace fast_reconnect

LL_REGISTER_MOD(fast_reconnect::FastReconnectMod, fast_reconnect::FastReconnectMod::getInstance());
